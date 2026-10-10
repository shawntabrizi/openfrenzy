/*
HOST_BINK.C

Bink video decoding on behalf of the guest (src/dsound/bink_host.c, which
implements the RAD Bink entry points the game calls). The game is linked
with RAD's Bink SDK (XBE region LIB:bink 0x00057980-0x000703a0); the port
decodes the same files with FFmpeg's libavformat (Bink demuxer) and
libavcodec (binkvideo, binkaudio_rdft/dct) instead.

openfrenzy, new (not from OpenCE: OpenCE's bink_null.c skips every movie).

The guest gives the host an open file descriptor (the disc image; guest and
host share descriptors, port/macos/host/host_syscall.c) and the byte range
of the .bik file inside it. The host reads the range with pread, so it needs
no copy of the file in guest memory (ATTRACT.bik is 72 MB).

Without FFmpeg (CMake option OPENFRENZY_BINK_FFMPEG off, or FFmpeg not
found) every open fails, and the game skips its videos as it did with
bink_null.c.
*/

#include "host.h"

#include <stdint.h>
#include <string.h>

/* the guest's view of the info words (src/dsound/bink_host.c) */
enum {
    _info_width,
    _info_height,
    _info_frames,
    _info_rate,
    _info_rate_divisor,
    _info_audio_channels,
    _info_audio_rate,
    _info_count
};

#if OPENFRENZY_BINK_FFMPEG

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define BINK_HANDLE_COUNT 8
#define BINK_IO_BUFFER 65536

struct bink {
    int descriptor;          /* a dup of the guest's */
    int64_t base;            /* the file's first byte in the descriptor */
    int64_t size;
    int64_t position;
    AVIOContext *io;
    AVFormatContext *format;
    AVCodecContext *video;
    AVCodecContext *audio;
    int video_index;
    int audio_index;
    int audio_channels;      /* of the PCM given to the guest: 1 or 2 */
    AVPacket *packet;
    AVFrame *frame;          /* the last decoded video frame */
    AVFrame *audio_frame;
    int have_frame;
    int ended;
    struct SwsContext *scale;
    int16_t *pcm;            /* decoded audio not yet read, interleaved */
    size_t pcm_bytes;
    size_t pcm_capacity;
};

static struct bink *binks[BINK_HANDLE_COUNT];
static pthread_mutex_t bink_lock = PTHREAD_MUTEX_INITIALIZER;

static struct bink *bink_from_handle(uint32_t handle) {
    struct bink *bink = NULL;

    pthread_mutex_lock(&bink_lock);
    if (handle >= 1 && handle <= BINK_HANDLE_COUNT)
        bink = binks[handle - 1];
    pthread_mutex_unlock(&bink_lock);
    return bink;
}

static int io_read(void *opaque, uint8_t *buffer, int size) {
    struct bink *bink = opaque;
    int64_t left = bink->size - bink->position;
    ssize_t count;

    if (left <= 0)
        return AVERROR_EOF;
    if (size > left)
        size = (int)left;
    count = pread(bink->descriptor, buffer, (size_t)size, bink->base + bink->position);
    if (count < 0)
        return AVERROR(EIO);
    if (count == 0)
        return AVERROR_EOF;
    bink->position += count;
    return (int)count;
}

static int64_t io_seek(void *opaque, int64_t offset, int whence) {
    struct bink *bink = opaque;
    int64_t target;

    switch (whence & ~AVSEEK_FORCE) {
    case AVSEEK_SIZE:
        return bink->size;
    case SEEK_SET:
        target = offset;
        break;
    case SEEK_CUR:
        target = bink->position + offset;
        break;
    case SEEK_END:
        target = bink->size + offset;
        break;
    default:
        return -1;
    }
    if (target < 0 || target > bink->size)
        return -1;
    bink->position = target;
    return target;
}

static void bink_free(struct bink *bink) {
    if (!bink)
        return;
    sws_freeContext(bink->scale);
    avcodec_free_context(&bink->video);
    avcodec_free_context(&bink->audio);
    av_frame_free(&bink->frame);
    av_frame_free(&bink->audio_frame);
    av_packet_free(&bink->packet);
    avformat_close_input(&bink->format);
    if (bink->io) {
        av_freep(&bink->io->buffer);
        avio_context_free(&bink->io);
    }
    if (bink->descriptor >= 0)
        close(bink->descriptor);
    free(bink->pcm);
    free(bink);
}

static AVCodecContext *open_decoder(AVStream *stream) {
    const AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
    AVCodecContext *context;

    if (!codec)
        return NULL;
    context = avcodec_alloc_context3(codec);
    if (!context)
        return NULL;
    if (avcodec_parameters_to_context(context, stream->codecpar) < 0 ||
        avcodec_open2(context, codec, NULL) < 0) {
        avcodec_free_context(&context);
        return NULL;
    }
    return context;
}

static int clamp16(float value) {
    int sample = (int)(value * 32767.0f);

    return sample > 32767 ? 32767 : sample < -32768 ? -32768 : sample;
}

/* one sample of the decoded frame as a float */
static float audio_sample(const AVFrame *frame, int channel, int index) {
    int channels = frame->ch_layout.nb_channels;

    switch (frame->format) {
    case AV_SAMPLE_FMT_FLT:
        return ((const float *)frame->data[0])[index * channels + channel];
    case AV_SAMPLE_FMT_FLTP:
        return ((const float *)frame->data[channel])[index];
    case AV_SAMPLE_FMT_S16:
        return ((const int16_t *)frame->data[0])[index * channels + channel] / 32768.0f;
    case AV_SAMPLE_FMT_S16P:
        return ((const int16_t *)frame->data[channel])[index] / 32768.0f;
    default:
        return 0.0f;
    }
}

/* appends a decoded audio frame to the PCM queue (16-bit, interleaved;
channels past the second are dropped) */
static void queue_audio(struct bink *bink, const AVFrame *frame) {
    int source_channels = frame->ch_layout.nb_channels;
    size_t bytes = (size_t)frame->nb_samples * bink->audio_channels * sizeof(int16_t);
    int16_t *out;
    int index, channel;

    if (source_channels <= 0 || frame->nb_samples <= 0)
        return;
    if (bink->pcm_bytes + bytes > bink->pcm_capacity) {
        size_t capacity = (bink->pcm_bytes + bytes) * 2;
        int16_t *grown = realloc(bink->pcm, capacity);

        if (!grown)
            return;
        bink->pcm = grown;
        bink->pcm_capacity = capacity;
    }
    out = (int16_t *)((uint8_t *)bink->pcm + bink->pcm_bytes);
    for (index = 0; index < frame->nb_samples; index++) {
        for (channel = 0; channel < bink->audio_channels; channel++) {
            int source = channel < source_channels ? channel : 0;

            *out++ = (int16_t)clamp16(audio_sample(frame, source, index));
        }
    }
    bink->pcm_bytes += bytes;
}

static void decode_audio_packet(struct bink *bink, const AVPacket *packet) {
    if (!bink->audio || avcodec_send_packet(bink->audio, packet) < 0)
        return;
    while (avcodec_receive_frame(bink->audio, bink->audio_frame) == 0) {
        queue_audio(bink, bink->audio_frame);
        av_frame_unref(bink->audio_frame);
    }
}

uint32_t host_bink_open(int32_t descriptor, int64_t offset, int64_t size, uint32_t *info) {
    struct bink *bink;
    const AVInputFormat *input = av_find_input_format("bink");
    unsigned char *buffer;
    AVStream *stream;
    uint32_t handle;
    int index;

    memset(info, 0, _info_count * sizeof(*info));
    if (descriptor < 0 || size <= 0)
        return 0;
    av_log_set_level(AV_LOG_ERROR);
    bink = calloc(1, sizeof(*bink));
    if (!bink)
        return 0;
    bink->descriptor = dup(descriptor);
    bink->base = offset;
    bink->size = size;
    bink->video_index = -1;
    bink->audio_index = -1;
    buffer = av_malloc(BINK_IO_BUFFER);
    if (bink->descriptor < 0 || !buffer)
        goto failed;
    bink->io = avio_alloc_context(buffer, BINK_IO_BUFFER, 0, bink, io_read, NULL, io_seek);
    if (!bink->io) {
        av_free(buffer);
        goto failed;
    }
    bink->format = avformat_alloc_context();
    if (!bink->format)
        goto failed;
    bink->format->pb = bink->io;
    bink->format->flags |= AVFMT_FLAG_CUSTOM_IO;
    if (avformat_open_input(&bink->format, NULL, input, NULL) < 0) {
        /* (avformat_open_input frees the context on failure) */
        host_logf(HOST_LOG_WARN, "bink: not a Bink file");
        goto failed;
    }
    if (avformat_find_stream_info(bink->format, NULL) < 0)
        goto failed;
    bink->video_index = av_find_best_stream(bink->format, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    bink->audio_index = av_find_best_stream(bink->format, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (bink->video_index < 0)
        goto failed;
    for (index = 0; index < (int)bink->format->nb_streams; index++) {
        if (index != bink->video_index && index != bink->audio_index)
            bink->format->streams[index]->discard = AVDISCARD_ALL;
    }
    stream = bink->format->streams[bink->video_index];
    bink->video = open_decoder(stream);
    if (!bink->video)
        goto failed;
    if (bink->audio_index >= 0) {
        bink->audio = open_decoder(bink->format->streams[bink->audio_index]);
        if (bink->audio)
            bink->audio_channels = bink->audio->ch_layout.nb_channels >= 2 ? 2 : 1;
        else
            host_logf(HOST_LOG_WARN, "bink: cannot decode the audio track; the video plays silent");
    }
    bink->packet = av_packet_alloc();
    bink->frame = av_frame_alloc();
    bink->audio_frame = av_frame_alloc();
    if (!bink->packet || !bink->frame || !bink->audio_frame)
        goto failed;

    info[_info_width] = (uint32_t)bink->video->width;
    info[_info_height] = (uint32_t)bink->video->height;
    /* (the Bink demuxer gives the header's frame count as the duration, in frames) */
    info[_info_frames] = stream->nb_frames > 0 ? (uint32_t)stream->nb_frames :
        stream->duration > 0 ? (uint32_t)stream->duration : 0;
    /* the Bink header's rate and divisor (the demuxer's time base is their inverse) */
    info[_info_rate] = (uint32_t)stream->time_base.den;
    info[_info_rate_divisor] = (uint32_t)stream->time_base.num;
    if (bink->audio) {
        info[_info_audio_channels] = (uint32_t)bink->audio_channels;
        info[_info_audio_rate] = (uint32_t)bink->audio->sample_rate;
    }

    pthread_mutex_lock(&bink_lock);
    for (handle = 1; handle <= BINK_HANDLE_COUNT; handle++) {
        if (!binks[handle - 1]) {
            binks[handle - 1] = bink;
            break;
        }
    }
    pthread_mutex_unlock(&bink_lock);
    if (handle > BINK_HANDLE_COUNT) {
        host_logf(HOST_LOG_ERROR, "bink: too many open videos");
        goto failed;
    }
    return handle;

failed:
    bink_free(bink);
    memset(info, 0, _info_count * sizeof(*info));
    return 0;
}

/* decodes up to the next video frame; the audio packets read on the way are
decoded into the PCM queue */
int32_t host_bink_decode_frame(uint32_t handle) {
    struct bink *bink = bink_from_handle(handle);
    int result;

    if (!bink)
        return 0;
    while (!bink->ended) {
        result = avcodec_receive_frame(bink->video, bink->frame);
        if (result == 0) {
            bink->have_frame = 1;
            return 1;
        }
        if (result == AVERROR_EOF) {
            bink->ended = 1;
            break;
        }
        result = av_read_frame(bink->format, bink->packet);
        if (result < 0) {
            /* the end of the file: drain the decoder */
            avcodec_send_packet(bink->video, NULL);
            if (avcodec_receive_frame(bink->video, bink->frame) == 0) {
                bink->have_frame = 1;
                return 1;
            }
            bink->ended = 1;
            break;
        }
        if (bink->packet->stream_index == bink->video_index)
            avcodec_send_packet(bink->video, bink->packet);
        else if (bink->packet->stream_index == bink->audio_index)
            decode_audio_packet(bink, bink->packet);
        av_packet_unref(bink->packet);
    }
    return 0;
}

/* the last decoded frame as 32-bit pixels, bytes B, G, R, A (D3DFMT_A8R8G8B8 /
X8R8G8B8 in memory; Bink's BINKSURFACE32) */
int32_t host_bink_copy_frame(uint32_t handle, void *destination, int32_t pitch, int32_t width, int32_t height) {
    struct bink *bink = bink_from_handle(handle);
    uint8_t *planes[4] = { destination, NULL, NULL, NULL };
    int strides[4] = { pitch, 0, 0, 0 };
    AVFrame *frame;

    if (!bink || !bink->have_frame || !destination || pitch <= 0)
        return 0;
    frame = bink->frame;
    if (width > frame->width)
        width = frame->width;
    if (height > frame->height)
        height = frame->height;
    if (width <= 0 || height <= 0 || pitch < width * 4)
        return 0;
    bink->scale = sws_getCachedContext(bink->scale, frame->width, frame->height, frame->format,
        frame->width, frame->height, AV_PIX_FMT_BGRA, SWS_BILINEAR, NULL, NULL, NULL);
    if (!bink->scale)
        return 0;
    if (width == frame->width && height == frame->height) {
        sws_scale(bink->scale, (const uint8_t *const *)frame->data, frame->linesize, 0, frame->height,
            planes, strides);
    } else {
        /* a destination smaller than the video: convert whole, copy the corner */
        uint8_t *full[4] = { NULL };
        int full_strides[4] = { 0 };
        int row;

        if (av_image_alloc(full, full_strides, frame->width, frame->height, AV_PIX_FMT_BGRA, 16) < 0)
            return 0;
        sws_scale(bink->scale, (const uint8_t *const *)frame->data, frame->linesize, 0, frame->height,
            full, full_strides);
        for (row = 0; row < height; row++)
            memcpy((uint8_t *)destination + (size_t)row * pitch, full[0] + (size_t)row * full_strides[0],
                (size_t)width * 4);
        av_freep(&full[0]);
    }
    return 1;
}

/* moves up to size bytes (whole sample frames) of the queued audio */
int32_t host_bink_read_audio(uint32_t handle, void *buffer, int32_t size) {
    struct bink *bink = bink_from_handle(handle);
    size_t frame_bytes, count;

    if (!bink || !bink->audio_channels || !buffer || size <= 0)
        return 0;
    frame_bytes = (size_t)bink->audio_channels * sizeof(int16_t);
    count = bink->pcm_bytes < (size_t)size ? bink->pcm_bytes : (size_t)size;
    count -= count % frame_bytes;
    if (!count)
        return 0;
    memcpy(buffer, bink->pcm, count);
    memmove(bink->pcm, (uint8_t *)bink->pcm + count, bink->pcm_bytes - count);
    bink->pcm_bytes -= count;
    return (int32_t)count;
}

void host_bink_close(uint32_t handle) {
    struct bink *bink = NULL;

    pthread_mutex_lock(&bink_lock);
    if (handle >= 1 && handle <= BINK_HANDLE_COUNT) {
        bink = binks[handle - 1];
        binks[handle - 1] = NULL;
    }
    pthread_mutex_unlock(&bink_lock);
    bink_free(bink);
}

#else /* no FFmpeg: the null behaviour of OpenCE's bink_null.c */

uint32_t host_bink_open(int32_t descriptor, int64_t offset, int64_t size, uint32_t *info) {
    (void)descriptor;
    (void)offset;
    (void)size;
    memset(info, 0, _info_count * sizeof(*info));
    host_logf(HOST_LOG_WARN, "bink: this build has no FFmpeg (OPENFRENZY_BINK_FFMPEG off); videos are skipped");
    return 0;
}

int32_t host_bink_decode_frame(uint32_t handle) { (void)handle; return 0; }

int32_t host_bink_copy_frame(uint32_t handle, void *destination, int32_t pitch, int32_t width, int32_t height) {
    (void)handle; (void)destination; (void)pitch; (void)width; (void)height;
    return 0;
}

int32_t host_bink_read_audio(uint32_t handle, void *buffer, int32_t size) {
    (void)handle; (void)buffer; (void)size;
    return 0;
}

void host_bink_close(uint32_t handle) { (void)handle; }

#endif
