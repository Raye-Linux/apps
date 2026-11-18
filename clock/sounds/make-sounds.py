#!/usr/bin/env python3
#
# Writes the clock's alarm sounds as WAV files:
#
#   python3 make-sounds.py OUTDIR [NAME...]
#
# Each sound is one bar of a loop: the ring tool plays it over and over,
# so it starts and ends in silence and its gap is part of the rhythm.
# Same voice as the Raye sound theme (a soft glass tone over a small
# marimba body) and the same motif, the eye opening: E5, B5, G#5.
# Only the standard library, so the package build needs nothing more.

import math
import os
import struct
import sys
import wave

RATE = 32000
PEAK = 10 ** (-3.0 / 20)

E4, G4s, B4 = 329.63, 415.30, 493.88
C5s, E5, F5s, G5s, A5, B5 = 554.37, 659.26, 739.99, 830.61, 880.00, 987.77
C6s, E6, G6s, B6 = 1108.73, 1318.51, 1661.22, 1975.53


def glass(freq, length, decay):
    n = int(length * RATE)
    out = [0.0] * n
    w = 2 * math.pi * freq / RATE
    for i in range(n):
        t = i / RATE
        mod = 1.1 * math.exp(-t / 0.02) * math.sin(3.5 * w * i)
        out[i] = (math.sin(w * i + mod)
                  + 0.12 * math.sin(2 * w * i) * math.exp(-t / (decay * 0.6))
                  + 0.04 * math.sin(3 * w * i) * math.exp(-t / (decay * 0.3))) \
            * min(1.0, t / 0.004) * math.exp(-t / decay)
    return out


def wood(freq, length, decay):
    n = int(length * RATE)
    w = 2 * math.pi * freq / RATE
    return [(math.sin(w * i) * math.exp(-i / RATE / decay)
             + 0.25 * math.sin(3.93 * w * i) * math.exp(-i / RATE / 0.04)
             + 0.08 * math.sin(9.2 * w * i) * math.exp(-i / RATE / 0.012))
            * min(1.0, i / RATE / 0.002) for i in range(n)]


def bell(freq, length, decay):
    """Inharmonic partials of a small bell, the high ones short."""
    n = int(length * RATE)
    w = 2 * math.pi * freq / RATE
    parts = ((1.0, 1.0, 1.0), (2.0, 0.45, 0.6), (2.76, 0.3, 0.4), (5.4, 0.12, 0.15), (8.93, 0.05, 0.08))
    return [sum(a * math.sin(r * w * i) * math.exp(-i / RATE / (decay * d)) for r, a, d in parts)
            * min(1.0, i / RATE / 0.003) for i in range(n)]


def beep(freq, length):
    """A soft square-ish beep: odd harmonics, rounded ends."""
    n = int(length * RATE)
    w = 2 * math.pi * freq / RATE
    edge = int(0.012 * RATE)
    out = []
    for i in range(n):
        env = min(1.0, i / edge, (n - i) / edge)
        out.append((math.sin(w * i) + 0.25 * math.sin(3 * w * i) + 0.08 * math.sin(5 * w * i)) * env)
    return out


def note(freq, length=1.2, decay=0.25, body=0.35):
    g = glass(freq, length, decay)
    m = wood(freq, length, decay)
    return [(1 - body) * a + body * b for a, b in zip(g, m)]


def pad(freqs, length, attack, release):
    n = int(length * RATE)
    out = [0.0] * n
    for f in freqs:
        for k in (0.997, 1.003):
            w = 2 * math.pi * f * k / RATE
            for i in range(n):
                out[i] += math.sin(w * i)
    for i in range(n):
        t = i / RATE
        env = 0.5 - 0.5 * math.cos(math.pi * min(1.0, t / attack))
        tail = max(0.0, min(1.0, (length - t) / release))
        out[i] *= env * tail * tail / (2 * len(freqs))
    return out


def mix(length, parts):
    out = [0.0] * int(length * RATE)
    for start, gain, sig in parts:
        i0 = int(start * RATE)
        for i, v in enumerate(sig[:len(out) - i0]):
            out[i0 + i] += gain * v
    # the loop point is silent on both sides
    fade = int(0.04 * RATE)
    for i in range(fade):
        out[-1 - i] *= (i / fade) ** 2
    return out


def glow():
    """The motif twice, the second time an octave up, then a rest."""
    parts = []
    for k, f in enumerate((E5, B5, G5s, E6)):
        parts.append((0.15 * k, 0.9, note(f, 1.0, 0.22)))
    for k, f in enumerate((E6, B5 * 2, G6s)):
        parts.append((0.75 + 0.15 * k, 0.6, note(f, 0.9, 0.18)))
    return mix(2.4, parts)


def chimes():
    """Four bells walking down, the way a hall clock starts."""
    parts = [(0.35 * k, 0.8, bell(f, 1.6, 0.9)) for k, f in enumerate((G5s, F5s, E5, B4))]
    return mix(3.0, parts)


def marimba():
    """A quick wooden figure on the motif."""
    figure = (E5, G5s, B5, E6, B5, G5s, B5, E6)
    parts = [(0.11 * k, 1.0 if k % 4 == 0 else 0.75, wood(f, 0.5, 0.12)) for k, f in enumerate(figure)]
    return mix(1.6, parts)


def pulse():
    """The classic alarm: four short beeps and a rest."""
    parts = [(0.16 * k, 0.6, beep(1046.5, 0.09)) for k in range(4)]
    return mix(1.2, parts)


def dawn():
    """A slow warm chord with the motif rising over it: for waking gently."""
    parts = [(0.0, 0.7, pad((E4, G4s, B4), 3.2, 1.0, 1.2))]
    for k, f in enumerate((E5, G5s, B5, E6)):
        parts.append((0.6 + 0.45 * k, 0.45, note(f, 1.3, 0.35, 0.2)))
    return mix(3.6, parts)


SOUNDS = {"glow": glow, "chimes": chimes, "marimba": marimba, "pulse": pulse, "dawn": dawn}


def write(path, samples):
    peak = max(abs(v) for v in samples) or 1.0
    scale = PEAK / peak
    data = b"".join(struct.pack("<h", int(round(v * scale * 32767))) for v in samples)
    tmp = path + ".part"
    with wave.open(tmp, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(data)
    os.replace(tmp, path)


def main(argv):
    if len(argv) < 2:
        print("usage: make-sounds.py OUTDIR [NAME...]", file=sys.stderr)
        return 2
    out = argv[1]
    names = argv[2:] or list(SOUNDS)
    os.makedirs(out, exist_ok=True)
    for name in names:
        write(os.path.join(out, name + ".wav"), SOUNDS[name]())
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
