"""Free semantic C draft for one function via m2c (matt-kempster's decompiler).

Converts our capstone disassembly of the ROM bytes into GAS-style ARM assembly
that m2c's arm-gcc-c target parses, then runs vendor/m2c on it and prints the
resulting C. The draft is gcc-flavored pseudo-C: a comprehension scaffold that
shows the control flow, callee names, and data flow of a LARGE function. It is
NOT a matching candidate and will not compile under mwccarm as-is.

Conversion rules (see notes/m2c-setup.md for the m2c side):
  - each 4-byte word is decoded independently (no linear-sweep truncation at
    mid-function literal pools), then code vs pool data is classified by
    recursive descent from the entry point
  - branch targets become local labels (.L_<va>)
  - bl/blx targets are renamed to their resolved callee symbol (relocs.txt),
    else func_<addr>
  - pc-relative ldr becomes the "ldr rX, =value" pseudo-instruction: =SYM when
    the pool slot is a relocation, =0x<word> when it is a plain constant
  - add rX, pc, #imm (adr) becomes ldr rX, =<label-or-address>
  - pool/data words are emitted as labeled .word directives; words that are
    relocated (or point) into the function body become .word .L_<va> and their
    targets are classified as code (word-table switches)

Usage:
    python tools/m2c_draft.py --name func_020e2708
    python tools/m2c_draft.py --module ov002 --addr 0x020b7f2c
    python tools/m2c_draft.py --wl progress/wl_ab.jsonl --name func_020e2708
    python tools/m2c_draft.py --name X --dump-asm      # generated asm on stderr

--wl mode needs no extracted/ ROM: bytes come from the row's target_hex and
symbols/relocs from the committed config/ tree. Live mode reads the module
binary like every other tool.

Requires vendor/m2c: git clone https://github.com/matt-kempster/m2c vendor/m2c
"""
import argparse
import json
import pathlib
import re
import subprocess
import sys
import tempfile

from capstone import Cs, CS_ARCH_ARM, CS_MODE_ARM

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import relocs as R

REPO = pathlib.Path(__file__).resolve().parent.parent
M2C_PY = REPO / "vendor" / "m2c" / "m2c.py"

md = Cs(CS_ARCH_ARM, CS_MODE_ARM)

COND = r"(?:eq|ne|cs|hs|cc|lo|mi|pl|vs|vc|hi|ls|ge|lt|gt|le|al)"
B_RE = re.compile(rf"^b({COND})?$")
BL_RE = re.compile(rf"^blx?({COND})?$")
BX_RE = re.compile(rf"^bx({COND})?$")
LDR_RE = re.compile(rf"^ldr({COND})?$")           # plain word load only (can take =)
ANYLOAD_RE = re.compile(rf"^ldr(s?[bh])?({COND})?$")
RETBLK_RE = re.compile(rf"^(?:pop|ldm(?:ia|ib|da|db|fd|fa|ed|ea)?)({COND})?$")
PCREL_RE = re.compile(r"^(\w+),\[pc,#(-?0x[0-9a-fA-F]+|-?\d+)\]$")
ADRPC_RE = re.compile(r"^(\w+),pc,#(0x[0-9a-fA-F]+|\d+)$")
IMM_OP_RE = re.compile(r"^#(-?0x[0-9a-fA-F]+|-?\d+)$")
ADDPCPC_RE = re.compile(r"^pc,pc,(\w+),lsl#2$")


class M2CError(RuntimeError):
    pass


def sq(i):
    return i.op_str.replace(" ", "")


def decode_words(tgt, addr):
    """{offset: capstone insn or None} decoding every aligned word independently,
    so a mid-function literal pool cannot truncate the disassembly."""
    out = {}
    for off in range(0, len(tgt) - 3, 4):
        got = list(md.disasm(tgt[off:off + 4], addr + off))
        out[off] = got[0] if got else None
    return out


def _branch_target(i):
    """Absolute branch-target address of a b/bl/blx with an immediate operand."""
    m = IMM_OP_RE.match(sq(i))
    return int(m.group(1), 0) if m else None


def _ends_flow(i):
    """True if an (unconditional) instruction never falls through."""
    mn, op = i.mnemonic, sq(i)
    if mn == "b":
        return True
    if mn == "bx":
        return True
    m = RETBLK_RE.match(mn)
    if m and m.group(1) is None and "pc" in op:
        return True
    if mn in ("mov", "ldr") and op.startswith("pc,"):
        return True
    return False


def classify(tgt, addr, ins, relocs):
    """Split the function's words into code and data offsets.

    Recursive descent from offset 0. add pc,pc,rX,lsl#2 dispatch tables (the
    branch rows that follow) are walked explicitly; word-table switches are
    caught afterwards by treating any data word that is relocated (or points)
    into the function as a code entry point.
    """
    size = len(tgt) // 4 * 4
    code = set()

    def valid(off):
        return 0 <= off < size and off % 4 == 0 and ins.get(off) is not None

    def sweep(entry):
        work = [entry]
        while work:
            off = work.pop()
            while valid(off) and off not in code:
                i = ins[off]
                code.add(off)
                mn = i.mnemonic
                if BL_RE.match(mn) or B_RE.match(mn):
                    t = _branch_target(i)
                    if t is not None and 0 <= t - addr < size:
                        work.append(t - addr)
                elif ADDPCPC_RE.match(sq(i)) and mn.startswith("add"):
                    # switch dispatch: default branch + branch table follow
                    j = off + 4
                    while valid(j) and (B_RE.match(ins[j].mnemonic)
                                        or ("pc" in sq(ins[j])
                                            and RETBLK_RE.match(ins[j].mnemonic))):
                        code.add(j)
                        t = _branch_target(ins[j])
                        if t is not None and 0 <= t - addr < size:
                            work.append(t - addr)
                        j += 4
                    break
                if _ends_flow(i):
                    break
                off += 4

    sweep(0)
    # word-table switches: a data word relocated (or pointing) into the function
    # is a case-block address; classify its target as code and repeat to fixpoint
    while True:
        entries = []
        for off in range(0, size, 4):
            if off in code:
                continue
            e = relocs.get(addr + off)
            w = e[1] if e else int.from_bytes(tgt[off:off + 4], "little")
            t = w - addr
            if 0 <= t < size and t % 4 == 0 and t not in code and valid(t):
                entries.append(t)
        if not entries:
            break
        for t in entries:
            sweep(t)
    return code


def build_asm(name, addr, tgt, relocs, syms, window=None):
    """The GAS-syntax translation unit m2c parses: one function, local labels,
    resolved callees, =pool pseudo-loads, labeled .word data.

    `window` optionally extends tgt with the bytes that follow the function:
    mwccarm sometimes places the literal pool just past the symbol-table size,
    so a pc-relative load may reference beyond tgt. Such slots only need their
    VALUE (the load is rewritten to =VAL); nothing is emitted for them."""
    window = window if window is not None and len(window) >= len(tgt) else tgt
    size = len(tgt) // 4 * 4
    ins = decode_words(tgt, addr)
    code = classify(tgt, addr, ins, relocs)

    def callee(va, fallback_target):
        e = relocs.get(va)
        if e:
            return R.name_for_reloc(e, syms)
        return f"func_{fallback_target:08x}"

    labels = set()
    rewrites = {}
    for off in sorted(code):
        i = ins[off]
        mn, s = i.mnemonic, sq(i)
        bl, b = BL_RE.match(mn), B_RE.match(mn)
        if bl or b:
            t = _branch_target(i)
            if t is None:
                continue                               # blx reg etc: keep as-is
            if 0 <= t - addr < size and (t - addr) in code:
                labels.add(t - addr)
                rewrites[off] = f"{mn} .L_{t:08x}"
            else:
                rewrites[off] = f"{mn} {callee(addr + off, t)}"
            continue
        pm = PCREL_RE.match(s)
        if pm and ANYLOAD_RE.match(mn):
            pool = off + 8 + int(pm.group(2), 0)
            if not (0 <= pool <= len(window) - 4):
                raise M2CError(f"pc-relative load at +0x{off:x} references outside "
                               f"the function (+0x{pool:x})")
            if not LDR_RE.match(mn):                   # ldrh/ldrb from pool: label ref
                if pool >= size:
                    raise M2CError(f"non-word pc-relative load at +0x{off:x} "
                                   f"references past the function (+0x{pool:x})")
                labels.add(pool)
                rewrites[off] = f"{mn} {pm.group(1)}, .L_{addr + pool:08x}"
                continue
            e = relocs.get(addr + pool)
            if e:
                val = R.name_for_reloc(e, syms)
            else:
                val = f"0x{int.from_bytes(window[pool:pool + 4], 'little'):x}"
            rewrites[off] = f"{mn} {pm.group(1)}, ={val}"
            continue
        am = ADRPC_RE.match(s)
        if am and mn == "add":
            t = off + 8 + int(am.group(2), 0)
            if 0 <= t < size:
                labels.add(t)
                rewrites[off] = f"ldr {am.group(1)}, =.L_{addr + t:08x}"
            else:
                rewrites[off] = f"ldr {am.group(1)}, =0x{addr + t:x}"
            continue

    # every referenced data word needs a label; give unreferenced ones one too
    # (harmless, and keeps adr/word-table references resolvable)
    for off in range(0, size, 4):
        if off not in code:
            labels.add(off)

    lines = [".text", ".code 32", f".globl {name}", f"{name}:"]
    for off in range(0, size, 4):
        if off in labels:
            lines.append(f".L_{addr + off:08x}:")
        if off in code:
            i = ins[off]
            lines.append("\t" + rewrites.get(off, f"{i.mnemonic} {i.op_str}"))
        else:
            e = relocs.get(addr + off)
            w = e[1] if e else int.from_bytes(tgt[off:off + 4], "little")
            if 0 <= w - addr < size and (w - addr) in code:
                lines.append(f"\t.word .L_{w:08x}")
            elif e:
                lines.append(f"\t.word {R.name_for_reloc(e, syms)}")
            else:
                lines.append(f"\t.word 0x{w:x}")
    if len(tgt) != size:
        sys.stderr.write(f"warning: {len(tgt) - size} trailing non-word bytes ignored\n")
    return "\n".join(lines) + "\n"


def run_m2c(asm_text):
    """(returncode, stdout, stderr) of vendor/m2c on the generated asm."""
    if not M2C_PY.is_file():
        raise M2CError("vendor/m2c not found. Run: "
                       "git clone https://github.com/matt-kempster/m2c vendor/m2c "
                       "(see notes/m2c-setup.md)")
    with tempfile.TemporaryDirectory() as td:
        p = pathlib.Path(td) / "func.s"
        p.write_text(asm_text, encoding="utf-8")
        r = subprocess.run([sys.executable, str(M2C_PY), "--target", "arm-gcc-c",
                            str(p)], capture_output=True, text=True, timeout=300)
    return r.returncode, r.stdout, r.stderr


def draft(name, addr, tgt, relocs, syms, window=None):
    """C draft text, or raise M2CError. The in-process API coddog.py --draft uses."""
    asm = build_asm(name, addr, tgt, relocs, syms, window=window)
    rc, out, err = run_m2c(asm)
    if rc != 0 or not out.strip():
        raise M2CError((err or out).strip() or f"m2c exited {rc}")
    return out


def reloc_file_for(label):
    """Committed relocs.txt path for a module label (arm9/ovNNN/itcm/dtcm)."""
    if label in ("arm9", "main"):
        return R.RELOCS
    if label in ("itcm", "dtcm"):
        return R.CFG / label / "relocs.txt"
    return R.CFG / "overlays" / label / "relocs.txt"


def resolve_wl(path, name):
    """(label, name, addr, size, tgt, window) from a worklist row: no ROM needed."""
    for line in pathlib.Path(path).read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        row = json.loads(line)
        if row["name"] == name:
            tgt = bytes.fromhex(row["target_hex"])
            return (row["module"], row["name"], int(row["addr"], 16),
                    int(row["size"], 16), tgt, tgt)
    sys.exit(f"{name}: not found in {path}")


def resolve_live(name, module, addr, size):
    """(label, name, addr, size, tgt, window) from module binaries, coddog-style.
    window pads past the symbol size for pools placed just after the function."""
    import modules as MOD
    import sweep
    for mod in MOD.modules():
        label = "arm9" if mod["name"] == "main" else mod["name"]
        if module and mod["name"] != ("main" if module == "arm9" else module):
            continue
        for fname, faddr, fsize in sweep.funcs(mod):
            if (name and fname == name) or (addr is not None and faddr == addr):
                fsize = size or fsize
                return (label, fname, faddr, fsize,
                        MOD.read_bytes(mod, faddr, fsize),
                        MOD.read_bytes(mod, faddr, fsize + 0x400))
    sys.exit(f"function not found ({name or (module, hex(addr or 0))})")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--name", default=None)
    ap.add_argument("--module", default=None)
    ap.add_argument("--addr", type=lambda x: int(x, 0), default=None)
    ap.add_argument("--size", type=lambda x: int(x, 0), default=None,
                    help="override the symbol-table size (live mode)")
    ap.add_argument("--wl", default=None,
                    help="read the function from a worklist JSONL (with --name); "
                         "uses the row's target_hex, so no extracted/ ROM is needed")
    ap.add_argument("--dump-asm", action="store_true",
                    help="print the generated GAS asm to stderr")
    args = ap.parse_args()
    if not args.name and args.addr is None:
        ap.error("need --name or --addr")

    if args.wl:
        if not args.name:
            ap.error("--wl needs --name")
        label, name, addr, size, tgt, window = resolve_wl(args.wl, args.name)
    else:
        label, name, addr, size, tgt, window = resolve_live(
            args.name, args.module, args.addr, args.size)

    relocs = R.load_relocs_file(reloc_file_for(label))
    syms = R.load_all_syms()
    try:
        asm = build_asm(name, addr, tgt, relocs, syms, window=window)
        if args.dump_asm:
            sys.stderr.write(asm)
        rc, out, err = run_m2c(asm)
    except M2CError as e:
        sys.exit(f"m2c_draft: {e}")
    if rc != 0 or not out.strip():
        sys.stderr.write(err or "")
        sys.exit(rc or 1)
    sys.stderr.write(f"[{label} {name} @ 0x{addr:08x} size 0x{size:x}] "
                     "gcc-flavored semantic draft, NOT a matching candidate\n")
    print(out, end="")


if __name__ == "__main__":
    main()
