// Adapted from ref/OpenCE-mac-zimm3rmann (github.com/zimm3rmann/halo-ce-ios-macos @ e55684a) port/macos/host/host_sdl.c (CC0). Renamed HALO_* to OPENFRENZY_*.
/*
HOST_SDL.C

SDL3 on behalf of the guest (guest/runtime/guest_sdl.c). SDL objects are
64-bit pointers, which the guest cannot hold; it gets small handles into the
table here instead.

SDL video runs on the main thread. Audio mixing runs on a worker with a
guest stack, and output returns to SDL's callback thread to avoid taking
SDL's stream lock recursively from a different thread.
*/

#include "host.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <SDL3/SDL.h>
#include <pthread.h>
#include <string.h>

#define HANDLE_COUNT 256

enum handle_type {
    _handle_free,
    _handle_window,
    _handle_context,
    _handle_gamepad,
    _handle_audio,
    _handle_joystick,
};

struct handle {
    int type;
    void *object;
};

static struct handle handles[HANDLE_COUNT];
static pthread_mutex_t handle_lock = PTHREAD_MUTEX_INITIALIZER;

static uint32_t handle_new(int type, void *object) {
    uint32_t index;

    if (!object)
        return 0;
    pthread_mutex_lock(&handle_lock);
    /* an object that already has a handle keeps it */
    for (index = 1; index < HANDLE_COUNT; index++) {
        if (handles[index].type == type && handles[index].object == object) {
            pthread_mutex_unlock(&handle_lock);
            return index;
        }
    }
    for (index = 1; index < HANDLE_COUNT; index++) {
        if (handles[index].type == _handle_free) {
            handles[index].type = type;
            handles[index].object = object;
            pthread_mutex_unlock(&handle_lock);
            return index;
        }
    }
    pthread_mutex_unlock(&handle_lock);
    host_logf(HOST_LOG_ERROR, "out of SDL handles");
    return 0;
}

/* openfrenzy: frees a handle (the object is closed by the caller) */
static void handle_free(uint32_t handle, int type) {
    if (handle == 0 || handle >= HANDLE_COUNT)
        return;
    pthread_mutex_lock(&handle_lock);
    if (handles[handle].type == type) {
        handles[handle].type = _handle_free;
        handles[handle].object = NULL;
    }
    pthread_mutex_unlock(&handle_lock);
}

static void *handle_get(uint32_t handle, int type) {
    void *object = NULL;

    if (handle == 0 || handle >= HANDLE_COUNT)
        return NULL;
    pthread_mutex_lock(&handle_lock);
    if (handles[handle].type == type)
        object = handles[handle].object;
    pthread_mutex_unlock(&handle_lock);
    return object;
}

static EGLDisplay metal_display;
static EGLContext metal_context;
static EGLSurface metal_surface;
static int requested_minor;
static SDL_Window *metal_window;
static SDL_MetalView metal_view;

/* ---------- general */

int host_sdl_init(uint32_t flags) { return SDL_Init((SDL_InitFlags)flags); }

int host_sdl_set_hint(const char *name, const char *value) { return SDL_SetHint(name, value); }

void host_sdl_get_error(char *buffer, uint32_t size) { SDL_strlcpy(buffer, SDL_GetError(), size); }

int64_t host_sdl_ticks(void) { return (int64_t)SDL_GetTicks(); }

int64_t host_sdl_thread_id(void) { return (int64_t)SDL_GetCurrentThreadID(); }

/* ---------- video */

#if defined(OPENFRENZY_IOS) && !defined(OPENFRENZY_IOS_MAC_CHECK)
/* SDL already requests the display's maximum rate through CADisplayLink.
   Its public callback API replaces that link for the optional 60 Hz mode.
   ANGLE's drawable/vsync wait still paces the game's main loop. */
static void SDLCALL ios_refresh_hint(void *unused) { (void)unused; }
#endif

uint32_t host_sdl_create_window(const char *title, int width, int height, int64_t flags) {
    (void)flags;
    const char *windowed = SDL_getenv("OPENFRENZY_WINDOWED");
    SDL_WindowFlags mode = SDL_WINDOW_METAL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (!windowed || !SDL_atoi(windowed)) mode |= SDL_WINDOW_FULLSCREEN;
    metal_window = SDL_CreateWindow(title, width > 0 ? width : 1280, height > 0 ? height : 960,
                                    mode);
    if (metal_window) {
        SDL_SyncWindow(metal_window);
#if defined(OPENFRENZY_IOS) && !defined(OPENFRENZY_IOS_MAC_CHECK)
        const SDL_DisplayMode *display = SDL_GetDesktopDisplayMode(SDL_GetDisplayForWindow(metal_window));
        const char *rate = SDL_getenv("OPENFRENZY_IOS_REFRESH_RATE");
        int maximum = display ? (int)SDL_roundf(display->refresh_rate) : 60;
        int requested = maximum;
        if (rate && !SDL_strcmp(rate, "60")) {
            int interval = SDL_max(1, maximum / 60);
            if (SDL_SetiOSAnimationCallback(metal_window, interval, ios_refresh_hint, NULL))
                requested = maximum / interval;
            else
                host_logf(HOST_LOG_WARN, "Cannot request 60 Hz: %s", SDL_GetError());
        } else if (rate && SDL_strcmp(rate, "120")) {
            host_logf(HOST_LOG_WARN, "OPENFRENZY_IOS_REFRESH_RATE accepts 60 or 120; using display maximum");
        }
        host_logf(HOST_LOG_INFO, "iOS refresh request %d Hz; display maximum %d Hz (system may reduce rate)",
                  requested, maximum);
#endif
        int w, h;
        SDL_GetWindowSizeInPixels(metal_window, &w, &h);
        host_logf(HOST_LOG_INFO, "Metal drawable %dx%d (%s)", w, h,
                  (mode & SDL_WINDOW_FULLSCREEN) ? "borderless fullscreen" : "windowed");
    }
    return handle_new(_handle_window, metal_window);
}

void host_sdl_window_size_in_pixels(uint32_t window, int *width, int *height) {
    SDL_Window *object = handle_get(window, _handle_window);

    *width = 0;
    *height = 0;
    if (object)
        SDL_GetWindowSizeInPixels(object, width, height);
}

int host_sdl_set_relative_mouse(uint32_t window, int enabled) {
    SDL_Window *object = handle_get(window, _handle_window);

    return object ? SDL_SetWindowRelativeMouseMode(object, enabled != 0) : 0;
}

int host_sdl_gl_set_attribute(int attribute, int value) {
    if (attribute == SDL_GL_CONTEXT_MINOR_VERSION)
        requested_minor = value;
    return 1;
}
uint32_t host_sdl_gl_create_context(uint32_t window) {
    SDL_Window *object = handle_get(window, _handle_window);
    if (!object || requested_minor > 0) {
        SDL_SetError("ANGLE Metal uses ES 3.0");
        return 0;
    }
    PFNEGLGETPLATFORMDISPLAYEXTPROC getDisplay =
        (void *)eglGetProcAddress("eglGetPlatformDisplayEXT");
    const EGLint displayAttributes[] = {0x3203, 0x3489, EGL_NONE};
    metal_display = getDisplay ? getDisplay(0x3202, (void *)0, displayAttributes) : EGL_NO_DISPLAY;
    EGLint major, minor, count;
    if (metal_display == EGL_NO_DISPLAY || !eglInitialize(metal_display, &major, &minor)) {
        SDL_SetError("Cannot initialize ANGLE Metal: %x", eglGetError());
        return 0;
    }
    const EGLint configAttributes[] = {EGL_SURFACE_TYPE,
                                       EGL_WINDOW_BIT,
                                       EGL_RENDERABLE_TYPE,
                                       EGL_OPENGL_ES3_BIT,
                                       EGL_RED_SIZE,
                                       8,
                                       EGL_GREEN_SIZE,
                                       8,
                                       EGL_BLUE_SIZE,
                                       8,
                                       EGL_ALPHA_SIZE,
                                       8,
                                       EGL_DEPTH_SIZE,
                                       24,
                                       EGL_STENCIL_SIZE,
                                       8,
                                       EGL_NONE};
    EGLConfig config;
    if (!eglChooseConfig(metal_display, configAttributes, &config, 1, &count) || !count)
        return 0;
    eglBindAPI(EGL_OPENGL_ES_API);
    const EGLint contextAttributes[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    metal_context = eglCreateContext(metal_display, config, EGL_NO_CONTEXT, contextAttributes);
    metal_view = SDL_Metal_CreateView(object);
    metal_surface = eglCreateWindowSurface(
        metal_display, config, (EGLNativeWindowType)SDL_Metal_GetLayer(metal_view), NULL);
    if (metal_context == EGL_NO_CONTEXT || metal_surface == EGL_NO_SURFACE) {
        SDL_SetError("Cannot create Metal surface: %x", eglGetError());
        return 0;
    }
    eglMakeCurrent(metal_display, metal_surface, metal_surface, metal_context);
#if defined(OPENFRENZY_IOS) && !defined(OPENFRENZY_IOS_MAC_CHECK)
    // Attach controls after SDL has installed the Metal view, otherwise
    // the drawable can cover the controller's UIKit overlay.
    extern void host_ios_controls_initialize(SDL_Window *);
    host_ios_controls_initialize(object);
#endif
    return handle_new(_handle_context, metal_context);
}
int host_sdl_gl_make_current(uint32_t window, uint32_t context) {
    (void)window;
    return eglMakeCurrent(metal_display, metal_surface, metal_surface,
                          handle_get(context, _handle_context));
}
int host_sdl_gl_set_swap_interval(int interval) { return eglSwapInterval(metal_display, interval); }
/* openfrenzy: frame pacing log. Every 10 s: frames presented, average fps, the
 * worst frame time (swap to swap), frames over 20 ms and the average time spent
 * in eglSwapBuffers. Always on: one line each 10 s. */
static void pacing_frame(uint64_t start, uint64_t end) {
    static uint64_t window_start, last, worst, swap_total;
    static unsigned frames, slow;
    if (last) {
        uint64_t frame = start - last;
        if (frame > worst)
            worst = frame;
        if (frame > 20000000)
            slow++;
        frames++;
        swap_total += end - start;
    } else
        window_start = start;
    last = start;
    if (start - window_start >= UINT64_C(10000000000)) {
        double seconds = (start - window_start) / 1e9;
        host_logf(HOST_LOG_INFO,
                  "frame pacing: %u frames in %.1f s, %.1f fps, worst %.1f ms, %u over 20 ms, swap %.2f ms avg",
                  frames, seconds, frames / seconds, worst / 1e6, slow,
                  frames ? swap_total / 1e6 / frames : 0.0);
        window_start = start;
        worst = swap_total = 0;
        frames = slow = 0;
    }
}
int host_sdl_gl_swap_window(uint32_t window) {
    (void)window;
    uint64_t start = SDL_GetTicksNS();
    int result = eglSwapBuffers(metal_display, metal_surface);
    uint64_t end = SDL_GetTicksNS();
    pacing_frame(start, end);
    host_perf_frame((end - start) / 1e6);
    return result;
}

int host_sdl_poll_event(void *event) {
    SDL_Event host_event;

    if (!SDL_PollEvent(&host_event))
        return 0;
    /* the layouts agree except for the pointers of text, drop and user
    events, which the guest does not read */
    memcpy(event, &host_event, sizeof(host_event));
    return 1;
}

/* openfrenzy: the guest's SDL_PushEvent (debug.test_keys); the same layout
rule as host_sdl_poll_event, so only pointer-free events may come here */
int host_sdl_push_event(const void *event) {
    SDL_Event host_event;

    memcpy(&host_event, event, sizeof(host_event));
    return SDL_PushEvent(&host_event) ? 1 : 0;
}

/* ---------- gamepads */

int host_sdl_get_gamepads(uint32_t *ids, int capacity) {
    int count = 0, index;
    SDL_JoystickID *list = SDL_GetGamepads(&count);

    if (!list)
        return 0;
    if (count > capacity)
        count = capacity;
    for (index = 0; index < count; index++)
        ids[index] = list[index];
    SDL_free(list);
    return count;
}

uint32_t host_sdl_open_gamepad(uint32_t id) {
    SDL_Gamepad *gamepad = SDL_OpenGamepad((SDL_JoystickID)id);

    if (gamepad)
        host_logf(HOST_LOG_INFO, "gamepad %u: %s (type %d, %04x:%04x)", (unsigned)id,
                  SDL_GetGamepadName(gamepad), (int)SDL_GetGamepadType(gamepad),
                  SDL_GetGamepadVendor(gamepad), SDL_GetGamepadProduct(gamepad));
    return handle_new(_handle_gamepad, gamepad);
}

uint32_t host_sdl_gamepad_from_id(uint32_t id) {
    return handle_new(_handle_gamepad, SDL_GetGamepadFromID((SDL_JoystickID)id));
}

int host_sdl_gamepad_axis(uint32_t gamepad, int axis) {
    SDL_Gamepad *object = handle_get(gamepad, _handle_gamepad);

    return object ? SDL_GetGamepadAxis(object, (SDL_GamepadAxis)axis) : 0;
}

int host_sdl_gamepad_button(uint32_t gamepad, int button) {
    SDL_Gamepad *object = handle_get(gamepad, _handle_gamepad);

    return object ? SDL_GetGamepadButton(object, (SDL_GamepadButton)button) : 0;
}

int host_sdl_gamepad_type(uint32_t gamepad) {
    SDL_Gamepad *object = handle_get(gamepad, _handle_gamepad);

    return object ? SDL_GetGamepadType(object) : SDL_GAMEPAD_TYPE_UNKNOWN;
}

int host_sdl_rumble_gamepad(uint32_t gamepad, uint32_t low, uint32_t high, uint32_t milliseconds) {
    SDL_Gamepad *object = handle_get(gamepad, _handle_gamepad);

    return object ? SDL_RumbleGamepad(object, (Uint16)low, (Uint16)high, milliseconds) : 0;
}

/* openfrenzy: closes a gamepad that the guest opened (hot-plug: SDL keeps a removed
 * pad's SDL_Gamepad open until it is closed) and frees its handle */
void host_sdl_close_gamepad(uint32_t gamepad) {
    SDL_Gamepad *object = handle_get(gamepad, _handle_gamepad);

    if (!object)
        return;
    handle_free(gamepad, _handle_gamepad);
    SDL_CloseGamepad(object);
}

/* ---------- openfrenzy: virtual gamepads (debug.virtual_pads, src/runtime/sdl_platform.c)
 *
 * The guest's SDL_VirtualJoystickDesc has 32-bit pointers and guest callbacks, which SDL
 * cannot call, so the guest passes the plain fields and the host sets its own callbacks:
 * Rumble logs each request the game sends (XInputSetState -> SDL_RumbleGamepad, and SDL's
 * own stop when a rumble's duration ends). */
struct virtual_pad {
    SDL_JoystickID id;
};

static bool SDLCALL virtual_pad_rumble(void *userdata, Uint16 low, Uint16 high) {
    struct virtual_pad *pad = userdata;

    host_logf(HOST_LOG_INFO, "virtual gamepad %u: rumble low %u high %u", (unsigned)pad->id,
              (unsigned)low, (unsigned)high);
    return true;
}

static void SDLCALL virtual_pad_cleanup(void *userdata) { SDL_free(userdata); }

uint32_t host_sdl_attach_virtual_joystick(int type, int naxes, int nbuttons, uint32_t button_mask,
                                          uint32_t axis_mask, const char *name) {
    struct virtual_pad *pad = SDL_calloc(1, sizeof(*pad));
    SDL_VirtualJoystickDesc desc;

    if (!pad)
        return 0;
    SDL_INIT_INTERFACE(&desc);
    desc.type = (Uint16)type;
    desc.naxes = (Uint16)naxes;
    desc.nbuttons = (Uint16)nbuttons;
    desc.button_mask = button_mask;
    desc.axis_mask = axis_mask;
    desc.name = name;
    desc.userdata = pad;
    desc.Rumble = virtual_pad_rumble;
    desc.Cleanup = virtual_pad_cleanup;
    pad->id = SDL_AttachVirtualJoystick(&desc);
    if (!pad->id) {
        SDL_free(pad);
        return 0;
    }
    return pad->id;
}

int host_sdl_detach_virtual_joystick(uint32_t id) {
    return SDL_DetachVirtualJoystick((SDL_JoystickID)id) ? 1 : 0;
}

uint32_t host_sdl_open_joystick(uint32_t id) {
    return handle_new(_handle_joystick, SDL_OpenJoystick((SDL_JoystickID)id));
}

void host_sdl_close_joystick(uint32_t joystick) {
    SDL_Joystick *object = handle_get(joystick, _handle_joystick);

    if (!object)
        return;
    handle_free(joystick, _handle_joystick);
    SDL_CloseJoystick(object);
}

int host_sdl_set_joystick_virtual_axis(uint32_t joystick, int axis, int value) {
    SDL_Joystick *object = handle_get(joystick, _handle_joystick);

    return object ? SDL_SetJoystickVirtualAxis(object, axis, (Sint16)value) : 0;
}

int host_sdl_set_joystick_virtual_button(uint32_t joystick, int button, int down) {
    SDL_Joystick *object = handle_get(joystick, _handle_joystick);

    return object ? SDL_SetJoystickVirtualButton(object, button, down != 0) : 0;
}

/* ---------- audio */

/* SDL calls audio_callback on its own audio thread, which cannot run guest
code; it passes each request to the stream's thread (audio_thread), which
can, and waits for it to be done */
struct audio_binding {
    uint32_t handle;
    uint32_t callback;
    uint32_t userdata;
    pthread_mutex_t lock;
    pthread_cond_t requested;
    pthread_cond_t done;
    int pending;
    int additional;
    int total;
    unsigned char *staging;
    size_t staging_length;
    size_t staging_capacity;
    /* OPENFRENZY_DSOUND_LOG: the longest callback (handoff to the guest, its
     * mix and the submission) of the last 10 s (audio_handoff_log) */
    int log;
    uint64_t log_window_start, log_longest;
    unsigned log_callbacks;
};
static struct audio_binding *audio_bindings[HANDLE_COUNT];

static void audio_handoff_log(struct audio_binding *binding, uint64_t start, uint64_t end) {
    if (!binding->log_window_start)
        binding->log_window_start = start;
    if (end - start > binding->log_longest)
        binding->log_longest = end - start;
    binding->log_callbacks++;
    if (end - binding->log_window_start >= UINT64_C(10000000000)) {
        host_logf(HOST_LOG_INFO, "audio handoff: %u device callbacks in %.2f s, longest %.2f ms",
                  binding->log_callbacks, (end - binding->log_window_start) / 1e9,
                  binding->log_longest / 1e6);
        binding->log_window_start = end;
        binding->log_longest = 0;
        binding->log_callbacks = 0;
    }
}

static void *audio_thread(void *context) {
    struct audio_binding *binding = context;

    pthread_mutex_lock(&binding->lock);
    for (;;) {
        int additional, total;

        while (!binding->pending)
            pthread_cond_wait(&binding->requested, &binding->lock);
        additional = binding->additional;
        total = binding->total;
        pthread_mutex_unlock(&binding->lock);
        host_call_guest(binding->callback, binding->userdata, binding->handle, (uint32_t)additional,
                        (uint32_t)total);
        pthread_mutex_lock(&binding->lock);
        binding->pending = 0;
        pthread_cond_signal(&binding->done);
    }
    return NULL;
}

static void SDLCALL audio_callback(void *userdata, SDL_AudioStream *stream, int additional,
                                   int total) {
    struct audio_binding *binding = userdata;
    uint64_t start = binding->log ? SDL_GetTicksNS() : 0;

    (void)stream;
    pthread_mutex_lock(&binding->lock);
    binding->additional = additional;
    binding->total = total;
    binding->pending = 1;
    pthread_cond_signal(&binding->requested);
    while (binding->pending)
        pthread_cond_wait(&binding->done, &binding->lock);
    /* SDL invokes its callback with the stream locked. The guest mixer runs
     * on a different thread, so its output is staged and submitted here on
     * the callback thread. Calling SDL_PutAudioStreamData from that worker
     * would deadlock on SDL's stream lock while this callback waited. */
    if (binding->staging_length) {
        if (!SDL_PutAudioStreamData(stream, binding->staging, (int)binding->staging_length))
            host_logf(HOST_LOG_ERROR, "Audio submission failed: %s", SDL_GetError());
        binding->staging_length = 0;
    }
    pthread_mutex_unlock(&binding->lock);
    if (binding->log)
        audio_handoff_log(binding, start, SDL_GetTicksNS());
}

uint32_t host_sdl_open_audio_stream(uint32_t device, const void *spec, uint32_t callback,
                                    uint32_t userdata) {
    struct audio_binding *binding = SDL_calloc(1, sizeof(*binding));
    SDL_AudioStream *stream;
    if (!binding)
        return 0;

    binding->callback = callback;
    binding->userdata = userdata;
    pthread_mutex_init(&binding->lock, NULL);
    pthread_cond_init(&binding->requested, NULL);
    pthread_cond_init(&binding->done, NULL);
    stream = SDL_OpenAudioDeviceStream((SDL_AudioDeviceID)device, spec,
                                       callback ? audio_callback : NULL, binding);
    if (!stream) {
        SDL_free(binding);
        return 0;
    }
    binding->log = SDL_getenv("OPENFRENZY_DSOUND_LOG") != NULL;
    if (binding->log) {
        SDL_AudioDeviceID opened = SDL_GetAudioStreamDevice(stream);
        SDL_AudioSpec format;
        int frames = 0;

        if (SDL_GetAudioDeviceFormat(opened, &format, &frames))
            host_logf(HOST_LOG_INFO,
                      "audio device '%s': %d Hz, %d channels, %s, %d frames a buffer; the game's stream: "
                      "%d Hz, %d channels, %s",
                      SDL_GetAudioDeviceName(opened), format.freq, format.channels,
                      SDL_GetAudioFormatName(format.format), frames, ((const SDL_AudioSpec *)spec)->freq,
                      ((const SDL_AudioSpec *)spec)->channels,
                      SDL_GetAudioFormatName(((const SDL_AudioSpec *)spec)->format));
        else
            host_logf(HOST_LOG_ERROR, "audio device format unknown: %s", SDL_GetError());
    }
    /* the device starts paused, so no callback can run before this */
    binding->handle = handle_new(_handle_audio, stream);
    audio_bindings[binding->handle] = binding;
    if (callback && host_native_thread_create(audio_thread, binding, 256 * 1024) != 0)
        host_fatal("cannot start the audio thread");
    return binding->handle;
}

int host_sdl_put_audio_stream_data(uint32_t stream, const void *data, int length) {
    if (stream >= HANDLE_COUNT || length < 0)
        return 0;
    struct audio_binding *binding = audio_bindings[stream];
    if (!binding)
        return 0;
    pthread_mutex_lock(&binding->lock);
    size_t needed = binding->staging_length + (size_t)length;
    if (needed > binding->staging_capacity) {
        size_t capacity = (needed + 16383) & ~(size_t)16383;
        void *buffer = SDL_realloc(binding->staging, capacity);
        if (!buffer) {
            pthread_mutex_unlock(&binding->lock);
            return 0;
        }
        binding->staging = buffer;
        binding->staging_capacity = capacity;
    }
    memcpy(binding->staging + binding->staging_length, data, (size_t)length);
    binding->staging_length = needed;
    pthread_mutex_unlock(&binding->lock);
    return 1;
}

int host_sdl_resume_audio_stream_device(uint32_t stream) {
    SDL_AudioStream *object = handle_get(stream, _handle_audio);

    return object ? SDL_ResumeAudioStreamDevice(object) : 0;
}

/* ---------- openfrenzy additions (the OpenCE layer at 231708d calls these) */

/* dsound_sdl.c destroys its stream when the output device changes. The audio
 * binding owns a worker thread that waits for the stream's callback, so the
 * stream is only paused here, and the binding stays (a small leak per device
 * change). */
void host_sdl_destroy_audio_stream(uint32_t stream) {
    SDL_AudioStream *object = handle_get(stream, _handle_audio);

    if (object)
        SDL_PauseAudioStreamDevice(object);
}

int host_sdl_scancode_from_name(const char *name) { return (int)SDL_GetScancodeFromName(name); }

void host_sdl_scancode_name(int scancode, char *buffer, uint32_t size) {
    if (size)
        SDL_strlcpy(buffer, SDL_GetScancodeName((SDL_Scancode)scancode), size);
}
