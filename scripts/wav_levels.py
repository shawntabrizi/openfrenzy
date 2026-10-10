#!/usr/bin/env python3
"""Measure the level of a 16-bit PCM WAV file (OPENFRENZY_DSOUND_CAPTURE).

Usage: python3 -I scripts/wav_levels.py <file.wav> [--window SECONDS]

Prints for the whole file: duration, RMS and peak (dBFS), the share of samples at
full scale (|x| >= 32767, "clipped"), the share of samples that are not silent
(|x| > 32, about -60 dBFS), and the share of 1-second windows whose RMS is under
-60 dBFS. With --window it also prints RMS and peak for each window.
Standard library only (wave, array, math)."""

import argparse
import array
import math
import sys
import wave


def dbfs(value):
    return 20.0 * math.log10(value / 32768.0) if value > 0 else float("-inf")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("path")
    parser.add_argument("--window", type=float, default=0.0)
    args = parser.parse_args()

    with wave.open(args.path, "rb") as wav:
        if wav.getsampwidth() != 2:
            sys.exit("only 16-bit PCM is handled")
        rate, channels = wav.getframerate(), wav.getnchannels()
        data = array.array("h", wav.readframes(wav.getnframes()))
    if sys.byteorder == "big":
        data.byteswap()
    count = len(data)
    if not count:
        sys.exit("no samples")

    square_sum = 0.0
    peak = clipped = non_silent = 0
    for value in data:
        magnitude = -value if value < 0 else value
        square_sum += value * value
        if magnitude > peak:
            peak = magnitude
        if magnitude >= 32767:
            clipped += 1
        if magnitude > 32:
            non_silent += 1

    window = int(rate * channels)
    silent_windows = windows = 0
    for start in range(0, count - window + 1, window):
        chunk = data[start:start + window]
        rms = math.sqrt(sum(v * v for v in chunk) / len(chunk))
        windows += 1
        silent_windows += dbfs(rms) < -60.0
    print(f"file: {args.path}")
    print(f"format: {rate} Hz, {channels} channels, 16-bit; {count // channels / rate:.1f} s")
    print(f"rms: {dbfs(math.sqrt(square_sum / count)):.1f} dBFS")
    print(f"peak: {dbfs(peak):.2f} dBFS ({peak})")
    print(f"clipped (|x| >= 32767): {100.0 * clipped / count:.4f} % of samples ({clipped})")
    print(f"non-silent (|x| > 32): {100.0 * non_silent / count:.1f} % of samples")
    print(f"silent 1 s windows (rms < -60 dBFS): {silent_windows} of {windows}")

    if args.window > 0:
        step = max(1, int(args.window * rate)) * channels
        for start in range(0, count, step):
            chunk = data[start:start + step]
            rms = math.sqrt(sum(v * v for v in chunk) / len(chunk))
            chunk_peak = max(abs(v) for v in chunk)
            print(f"{start / channels / rate:7.1f} s  rms {dbfs(rms):7.1f} dBFS  peak {dbfs(chunk_peak):7.2f} dBFS")


if __name__ == "__main__":
    main()
