#!/usr/bin/env python3
"""Deterministic synthetic cabinet + room impulse-response library.

72 usable, *synthetic* 32-bit floating point WAV IRs at 96 kHz / 5 seconds.
A 768-tap excerpt of each IR is embedded into the VST3 for instant switching;
the complete WAVs ship in Contents/Resources/CabinetIRLibrary and can be used
with other full-length convolution processors. No meaningless padding.
"""
import argparse
import array
import json
import math
import random
import struct
import sys
from pathlib import Path

RATE = 96000
SECONDS = 5
PREVIEW_TAPS = 768
CABINETS = ["1x12_Open", "4x12_Modern", "2x12_Oversized"]
MICROPHONES = ["Dynamic57", "Ribbon121", "Condenser87", "Dynamic421"]
POSITIONS = ["Center", "CapEdge", "Cone", "Mid", "Edge", "Room"]

def profile_info(index):
    cab = index // 24
    mic = (index // 6) % 4
    pos = index % 6
    label = f"{CABINETS[cab]}__{MICROPHONES[mic]}__{POSITIONS[pos]}"
    return cab, mic, pos, label

def generate_ir(index, count):
    """A short cabinet-filter attack and a diffuse, gently damped room decay."""
    cab, mic, pos, _ = profile_info(index)
    seed = 428039 + (cab + 1)*9176 + (mic + 1)*5137 + (pos + 1)*331
    rng = random.Random(seed)
    # Cabinet acoustics: smaller cabinet loses low-mid bloom sooner, ribbon
    # responses are more damped, and edge/room captures are more diffuse.
    cab_tau = [0.0038, 0.0045, 0.0060][cab] * (1.0 + pos * .15)
    rt60 = [2.4, 3.0, 3.7][cab] + pos * .24 + (0.20 if mic == 1 else 0.0)
    cab_step = math.exp(-1.0 / (RATE * cab_tau))
    room_step = math.exp(math.log(.001) / (RATE * rt60))
    cab_env = 1.0
    room_env = 1.0
    smooth = 0.0
    # Positions change direct/reflected balance; each seed changes the combs.
    direct = (0.78 - .052*pos) * [1.0, .91, 1.07, .98][mic]
    low = [.53, .35, .62, .46][mic]
    reflections_ms = [8.3, 18.6, 31.1, 47.7, 76.4, 113.3, 183.7, 317.0, 580.5]
    reflections = {}
    for i, ms in enumerate(reflections_ms):
        offset = int(RATE * .001 * (ms*(1+.038*pos) + (mic*1.7)))
        reflections[offset] = (0.12 * (1 if i % 2 == 0 else -1)
                               * (0.82 ** i) * (1 + pos*.07))
    values = array.array("f")
    add = values.append
    for i in range(count):
        # All 72 captures have a distinct diffuse response, not empty samples.
        noise = 2.0*rng.random() - 1.0
        smooth = smooth*.87 + noise*.13
        cab_signal = ((1-low)*noise + low*smooth) * cab_env * (.095+.006*pos)
        room_signal = ((.25*noise+.75*smooth)*room_env
                       * (.0045 + .0015*pos)) if i > int(.010 * RATE) else 0.0
        sample = cab_signal + room_signal + reflections.get(i, 0.0)
        if i == 0:
            sample += direct
        add(max(-.98, min(.98, sample)))
        cab_env *= cab_step
        room_env *= room_step
    return values

def write_float_wav(path, samples):
    # IEEE float WAV, format code 3; decoded by JUCE and standard IR loaders.
    if sys.byteorder != "little":
        samples.byteswap()
    payload = samples.tobytes()
    hdr = struct.pack("<4sI4s4sIHHIIHH4sI",
                      b"RIFF", 36+len(payload), b"WAVE",
                      b"fmt ", 16, 3, 1, RATE, RATE*4, 4, 32,
                      b"data", len(payload))
    path.write_bytes(hdr+payload)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--dest", required=True, type=Path)
    parser.add_argument("--full", action="store_true",
                        help="Generate the complete ~132 MiB WAV library")
    args = parser.parse_args()
    args.dest.mkdir(parents=True, exist_ok=True)
    bank = args.dest / "CabinetIRLibrary"
    if args.full:
        bank.mkdir(parents=True, exist_ok=True)
    short_path = args.dest / "ir_bank_short.bin"
    manifest = []
    with short_path.open("wb") as short:
        for idx in range(72):
            _, _, _, label = profile_info(idx)
            preview = generate_ir(idx, PREVIEW_TAPS)
            if sys.byteorder != "little":
                preview.byteswap()
            short.write(preview.tobytes())
            manifest.append({"index": idx, "name": label, "file": label+".wav"})
            if args.full:
                full = generate_ir(idx, RATE*SECONDS)
                write_float_wav(bank/(label+".wav"), full)
    (args.dest/"manifest.json").write_text(
        json.dumps({"rate":RATE,"duration":SECONDS,"synthetic":True,
                    "profiles":manifest}, indent=2), encoding="utf8")
    expected = 72 * PREVIEW_TAPS * 4
    assert short_path.stat().st_size == expected
    if args.full:
        size = sum(p.stat().st_size for p in bank.glob("*.wav"))
        assert len(list(bank.glob("*.wav"))) == 72
        assert size > 105 * 1024 * 1024, size
        print(f"Generated 72 synthetic 96 kHz, 32-bit hybrid cabinet/room IRs: "
              f"{size / (1024*1024):.1f} MiB")
    else:
        print(f"Generated 72 native short FIR profiles ({expected} bytes)")

if __name__ == "__main__":
    main()
