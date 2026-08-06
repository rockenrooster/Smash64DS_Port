#!/usr/bin/env python3
"""
adpcm_offline_decode.py — Reference VADPCM decoder for SSB64 port debugging.

Reads /tmp/adpcm_dump.bin (captured by aADPCMdecImpl with SSB64_ADPCM_DUMP=1)
and decodes it independently using a from-spec reference implementation.
Compares the result to /tmp/adpcm_dump_output.bin (the port's actual output)
sample-by-sample and reports the first divergence.

If the port and reference outputs match → ADPCM decode is correct, look
elsewhere for the broadband-noise source.

If they diverge inside the FIRST frame → codebook layout / decoder bug.
If they diverge only at frame boundaries → state save/restore bug.

Dump format (header + body):
    [0..7]   magic "ADPCMDP1"
    [8..11]  flags    (u32 LE)
    [12..15] nbytes   (s32 LE)  — input bytes after header
    [16..17] in_off   (u16 LE)  — DMEM addr (informational)
    [18..19] out_off  (u16 LE)  — DMEM addr (informational)
    [20..31] reserved (12 bytes, zero)
    [32..287]      codebook = 8 × 2 × 8 s16 LE  (256 bytes)
    [288..319]     loop_state = 16 s16 LE       (32 bytes)
    [320..351]     prior_state = 16 s16 LE      (32 bytes)
    [352..)        input bytes (length = nbytes)
"""
import os
import struct
import sys


def clamp16(v: int) -> int:
    if v < -0x8000: return -0x8000
    if v >  0x7fff: return  0x7fff
    return v


def sign_extend_4bit(n: int) -> int:
    n &= 0xf
    return n - 16 if n & 0x8 else n


def decode_adpcm(flags: int, nbytes: int, book, loop_state, prior_state, input_bytes):
    """Reference VADPCM decode.  Returns the full output sample array
    including the 16-sample state prefix (matches our aADPCMdecImpl)."""
    out = []

    # Initial 16 samples
    if flags & 0x01:        # A_INIT
        out.extend([0] * 16)
    elif flags & 0x02:      # A_LOOP
        out.extend(loop_state[:16])
    else:                   # A_CONTINUE
        out.extend(prior_state[:16])

    in_idx = 0
    decoded = 0
    # Mirror the C port loop precisely: nbytes is decremented ONCE per outer
    # ADPCM-frame iteration (1 header + 2 sub-frames = 16 samples = 32 bytes).
    while decoded < nbytes:
        hdr = input_bytes[in_idx]; in_idx += 1
        shift = hdr >> 4
        table_index = hdr & 0xf
        tbl = book[table_index]   # tbl[0..1][0..7]

        # Two 8-sample sub-frames per ADPCM frame
        for sub in range(2):
            prev1 = out[-1]
            prev2 = out[-2]

            ins = [0] * 8
            if flags & 4:
                # 2-bit ADPCM (rare).
                for j in range(2):
                    b = input_bytes[in_idx]; in_idx += 1
                    for k in range(4):
                        v = (b >> (6 - k * 2)) & 0x3
                        # 2-bit sign extend
                        if v & 2: v |= ~3 & 0xffffffff
                        v = v if v < 0x80000000 else v - 0x100000000
                        ins[j * 4 + k] = v << shift
            else:
                # 4-bit ADPCM
                for j in range(4):
                    b = input_bytes[in_idx]; in_idx += 1
                    ins[j * 2]     = sign_extend_4bit(b >> 4) << shift
                    ins[j * 2 + 1] = sign_extend_4bit(b & 0xf) << shift

            for j in range(8):
                acc = tbl[0][j] * prev2 + tbl[1][j] * prev1 + (ins[j] << 11)
                for k in range(j):
                    acc += tbl[1][(j - k) - 1] * ins[k]
                acc >>= 11
                out.append(clamp16(acc))

        decoded += 16 * 2  # one outer iter = 16 output samples = 32 bytes

    return out


def parse_dump(path: str):
    with open(path, "rb") as f:
        data = f.read()
    if data[:8] != b"ADPCMDP1":
        raise SystemExit(f"bad magic: {data[:8]!r}")
    flags  = struct.unpack_from("<I", data, 8)[0]
    nbytes = struct.unpack_from("<i", data, 12)[0]
    in_off  = struct.unpack_from("<H", data, 16)[0]
    out_off = struct.unpack_from("<H", data, 18)[0]

    pos = 32
    book_flat = list(struct.unpack_from("<128h", data, pos)); pos += 256
    # Reshape: book_flat is rspa.adpcm_table[8][2][8] in C declaration order.
    book = [
        [
            book_flat[t * 16 + p * 8 : t * 16 + p * 8 + 8]
            for p in range(2)
        ]
        for t in range(8)
    ]
    loop_state  = list(struct.unpack_from("<16h", data, pos)); pos += 32
    prior_state = list(struct.unpack_from("<16h", data, pos)); pos += 32
    input_bytes = data[pos : pos + nbytes]
    if len(input_bytes) != nbytes:
        raise SystemExit(f"truncated input: have {len(input_bytes)} need {nbytes}")
    return {
        "flags": flags, "nbytes": nbytes,
        "in_off": in_off, "out_off": out_off,
        "book": book, "loop_state": loop_state,
        "prior_state": prior_state, "input_bytes": input_bytes,
    }


def main():
    dump_path = sys.argv[1] if len(sys.argv) > 1 else "/tmp/adpcm_dump.bin"
    output_path = sys.argv[2] if len(sys.argv) > 2 else "/tmp/adpcm_dump_output.bin"

    d = parse_dump(dump_path)
    print(f"== Dump {dump_path}")
    print(f"   flags=0x{d['flags']:02x} nbytes={d['nbytes']}  in=0x{d['in_off']:04x} out=0x{d['out_off']:04x}")
    print(f"   prior_state[0..7]={d['prior_state'][:8]}")
    print(f"   input_bytes[0..15]={list(d['input_bytes'][:16])}")

    # Print non-zero codebook entries to confirm layout
    print(f"   book[0][0]={d['book'][0][0]}")
    print(f"   book[0][1]={d['book'][0][1]}")
    print(f"   book[1][0]={d['book'][1][0]}")
    print(f"   book[1][1]={d['book'][1][1]}")

    # Reference decode
    ref_out = decode_adpcm(d["flags"], d["nbytes"], d["book"],
                           d["loop_state"], d["prior_state"], d["input_bytes"])
    print(f"\n== Reference decode produced {len(ref_out)} samples")
    print(f"   first 16: {ref_out[:16]}")
    print(f"   samples 16..31: {ref_out[16:32]}")

    # Load port output if present
    if os.path.exists(output_path):
        with open(output_path, "rb") as f:
            port_data = f.read()
        port_out = list(struct.unpack(f"<{len(port_data)//2}h", port_data))
        print(f"\n== Port output {output_path}: {len(port_out)} samples")
        print(f"   first 16: {port_out[:16]}")
        print(f"   samples 16..31: {port_out[16:32]}")

        # Compare
        n = min(len(ref_out), len(port_out))
        diffs = []
        for i in range(n):
            if ref_out[i] != port_out[i]:
                diffs.append((i, ref_out[i], port_out[i]))
                if len(diffs) >= 8:
                    break
        if not diffs:
            print(f"\n[OK] First {n} samples match exactly. ADPCM decode is correct.")
        else:
            print(f"\n[DIFF] First divergences (showing up to 8):")
            for i, r, p in diffs:
                frame_idx = (i - 16) // 16   # which 16-sample frame after state
                in_frame  = (i - 16) % 16
                print(f"   sample {i:5d} (frame {frame_idx}, pos {in_frame}): "
                      f"ref={r:+6d}  port={p:+6d}  delta={p-r:+6d}")
            # Show first frame side by side
            print(f"\n   First frame side-by-side (samples 16..31):")
            for i in range(16, 32):
                marker = "" if ref_out[i] == port_out[i] else "  ←"
                print(f"     [{i:3d}] ref={ref_out[i]:+6d}  port={port_out[i]:+6d}{marker}")
    else:
        print(f"\n[no port output file at {output_path}; running with dump only]")


if __name__ == "__main__":
    main()
