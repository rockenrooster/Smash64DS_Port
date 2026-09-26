"""MFP2 roster closure and memory accounting.

The actor mask is derived from the authoritative ``FTMotionDesc`` rows parsed
by ``mf_corpus.motion_tables``.  Asset-bank ownership and main-table users are
separate: Luigi can use Mario-owned clips, and Purin can use Kirby-owned
clips.  Optional victim/copy requirements use per-instance opponent masks so a
same-kind mirror remains an opponent.
"""

from __future__ import annotations

import pathlib
import re
import struct
import zlib

import mf_corpus as mc


CLS_ALWAYS = 0
CLS_VICTIM = 1
CLS_COPY = 2

RAW_AOBJ32 = 1
RAW_SPLINE = 2
EXPECTED_RAW_EXCEPTION_COUNT = 29

PACK_HEADER_BYTES = 48
KIND_ROW_BYTES = 16
PACK_ENTRY_BYTES = 28
PACK_RAW_ENTRY_BYTES = 20
RAW_ASSET_ALIGN_BYTES = 16

MFT1_MAGIC = 0x3154464D
MFT1_HEADER_BYTES = 12
MFT1_RECIP_COUNT = 256
MFT1_TABLE_HEADER_BYTES = 30

# ARM946E-S layout: NdsMfTable is a 212-byte, 4-aligned object and the
# NdsMfTables pointer grid is 312 bytes.  Keep this target accounting separate
# from the host checker, whose pointer width can differ.
ARM_NDS_MF_TABLE_BYTES = 212
ARM_NDS_MF_TABLES_BYTES = 312


def _align4(value: int) -> int:
    return (value + 3) & ~3


def kind_mask(kinds) -> int:
    mask = 0
    for kind in kinds:
        try:
            mask |= 1 << mc.KINDS.index(kind)
        except ValueError as exc:
            raise ValueError("unknown fighter kind %r" % (kind,)) from exc
    return mask


def main_user_masks(corpus) -> dict[int, int]:
    """Asset id -> kinds whose authoritative main motion table references it."""
    users: dict[int, int] = {}
    for kind, rows in corpus["tables"].items():
        if kind not in mc.KINDS:
            raise ValueError("motion table has unknown owner kind %r" % kind)
        bit = 1 << mc.KINDS.index(kind)
        for symbol, asset_id in rows:
            if asset_id is None:
                if symbol is not None:
                    raise ValueError("unmapped required motion %s for %s" % (symbol, kind))
                continue
            users[asset_id] = users.get(asset_id, 0) | bit
    return users


def classify_clip(corpus, owner_kind: str, asset_id: int) -> tuple[int, int]:
    """Return current MFP class and opponent mask for an owned clip."""
    cls, who = mc.classify(owner_kind, corpus["names"].get(asset_id))
    if cls not in ("victim", "copy"):
        return CLS_ALWAYS, 0
    kinds = (who,) if isinstance(who, str) else tuple(who)
    return (CLS_VICTIM if cls == "victim" else CLS_COPY), kind_mask(kinds)


def opponent_masks_for_slots(slots) -> tuple[int, ...]:
    """One opponent-kind mask per fighter instance, preserving mirror matches."""
    slots = tuple(slots)
    for kind in slots:
        if kind not in mc.KINDS:
            raise ValueError("unknown fighter kind %r" % kind)
    masks = []
    for index in range(len(slots)):
        mask = 0
        for other, kind in enumerate(slots):
            if other != index:
                mask |= 1 << mc.KINDS.index(kind)
        masks.append(mask)
    return tuple(masks)


def _entry_value(entry, key):
    if isinstance(entry, dict):
        return entry[key]
    return getattr(entry, key)


def select_compressed_entries(entries, slots):
    """Select unique MFP entries needed by this exact roster.

    Main-table users are unconditional.  CLS_ALWAYS entries conservatively
    include every clip from a selected owner bank (including currently folded
    item, taunt and pipe classes).  Optional victim/copy rows are included for
    an owner instance only when one of its actual opponents matches need_mask.
    """
    slots = tuple(slots)
    actor_mask = kind_mask(slots)
    opponent_masks = opponent_masks_for_slots(slots)
    selected = []
    seen = set()
    for entry in entries:
        asset_id = _entry_value(entry, "asset_id")
        owner = _entry_value(entry, "kind")
        cls = _entry_value(entry, "cls")
        need_mask = _entry_value(entry, "need_mask")
        main_mask = _entry_value(entry, "main_user_mask")
        if asset_id in seen:
            raise ValueError("duplicate compressed asset id 0x%x" % asset_id)
        seen.add(asset_id)
        if isinstance(owner, int):
            if owner < 0 or owner >= len(mc.KINDS):
                raise ValueError("invalid owner kind index %d" % owner)
            owner = mc.KINDS[owner]
        if owner not in mc.KINDS:
            raise ValueError("invalid owner kind %r" % owner)

        include = bool(main_mask & actor_mask)
        if not include and cls == CLS_ALWAYS:
            include = owner in slots
        elif not include and cls in (CLS_VICTIM, CLS_COPY):
            include = any(
                actor == owner and (need_mask & opponent_masks[index])
                for index, actor in enumerate(slots)
            )
        elif cls not in (CLS_ALWAYS, CLS_VICTIM, CLS_COPY):
            raise ValueError("invalid class %r for asset 0x%x" % (cls, asset_id))
        if include:
            selected.append(entry)
    return selected


def select_raw_exceptions(raw_entries, slots):
    """Select uncompressed AObj32/spline O2R resources for a roster."""
    slots = tuple(slots)
    actor_mask = kind_mask(slots)
    selected = []
    seen = set()
    for entry in raw_entries:
        asset_id = _entry_value(entry, "asset_id")
        owner = _entry_value(entry, "kind")
        main_mask = _entry_value(entry, "main_user_mask")
        if asset_id in seen:
            raise ValueError("duplicate raw exception id 0x%x" % asset_id)
        seen.add(asset_id)
        if isinstance(owner, int):
            if owner < 0 or owner >= len(mc.KINDS):
                raise ValueError("invalid raw owner kind index %d" % owner)
            owner = mc.KINDS[owner]
        if owner not in mc.KINDS:
            raise ValueError("invalid raw owner kind %r" % owner)
        if (main_mask & actor_mask) or (owner in slots):
            selected.append(entry)
    return selected


def resident_plan(entries, raw_entries, slots, tables_blob_bytes,
                  expanded_tables_bytes, entry_bytes=PACK_ENTRY_BYTES,
                  tables_object_bytes=ARM_NDS_MF_TABLES_BYTES):
    """Return selected rows and exact A7/ROM byte categories.

    The bank retains the MFT1 backing blob because NdsMfTables contains
    pointers into it, the expanded tables and NdsMfTables object, selected
    28-byte rows and 4-aligned streams. The raw block starts at a 16-byte
    boundary, and every raw O2R payload uses ndsRelocAssetAllocSize's rounded
    allocation size. The full by-id table is read while planning and need not
    be retained: resident rows can be sorted by asset id.
    """
    selected = select_compressed_entries(entries, slots)
    raw = select_raw_exceptions(raw_entries, slots)
    ids = {_entry_value(entry, "asset_id") for entry in selected}
    collision = ids & {_entry_value(entry, "asset_id") for entry in raw}
    if collision:
        raise ValueError("asset id present as both MF and raw: 0x%x" % min(collision))

    stream_bytes = 0
    for entry in selected:
        if isinstance(entry, dict) and "mf_bytes" in entry:
            stream_bytes += _entry_value(entry, "mf_bytes")
        else:
            bits = _entry_value(entry, "stream_bits")
            stream_bytes += _align4((bits + 7) // 8)
    raw_payload_bytes = sum(_entry_value(entry, "payload_bytes") for entry in raw)
    raw_source_bytes = sum(_entry_value(entry, "source_bytes") for entry in raw)
    decoded_sizes = [
        _entry_value(entry, "decoded_bytes")
        for entry in selected
        if (isinstance(entry, dict) and "decoded_bytes" in entry)
        or hasattr(entry, "decoded_bytes")
    ]
    decode_scratch_bytes = max(decoded_sizes, default=0)
    descriptor_bytes = entry_bytes * len(selected)
    raw_descriptor_bytes = PACK_RAW_ENTRY_BYTES * len(raw)
    header_bytes = PACK_HEADER_BYTES
    fixed_bytes = (header_bytes + tables_blob_bytes + expanded_tables_bytes
                   + tables_object_bytes)
    pre_raw_bytes = fixed_bytes + descriptor_bytes + stream_bytes + raw_descriptor_bytes
    raw_alignment_padding = (-pre_raw_bytes) & (RAW_ASSET_ALIGN_BYTES - 1)
    raw_allocation_bytes = sum(
        _align_raw(_entry_value(entry, "payload_bytes")) for entry in raw
    )
    raw_asset_rounding_padding = raw_allocation_bytes - raw_payload_bytes
    required_a7_bytes = pre_raw_bytes + raw_alignment_padding + raw_allocation_bytes
    return {
        "slots": list(slots),
        "roster_mask": kind_mask(slots),
        "compressed_ids": sorted(ids),
        "raw_ids": sorted(_entry_value(entry, "asset_id") for entry in raw),
        "compressed_clips": len(selected),
        "raw_exceptions": len(raw),
        "tables_blob_bytes": tables_blob_bytes,
        "expanded_tables_bytes": expanded_tables_bytes,
        "tables_object_bytes": tables_object_bytes,
        "resident_header_bytes": header_bytes,
        "resident_entry_bytes": descriptor_bytes,
        "resident_raw_entry_bytes": raw_descriptor_bytes,
        "resident_stream_bytes": stream_bytes,
        "raw_payload_bytes": raw_payload_bytes,
        "raw_allocation_bytes": raw_allocation_bytes,
        "raw_asset_rounding_padding": raw_asset_rounding_padding,
        "raw_alignment_padding": raw_alignment_padding,
        "pre_raw_a7_bytes": pre_raw_bytes,
        "raw_source_bytes": raw_source_bytes,
        "decode_scratch_bytes": decode_scratch_bytes,
        "fixed_a7_bytes": fixed_bytes,
        "required_a7_bytes": required_a7_bytes,
        "peak_a7_with_one_decode_buffer_bytes": required_a7_bytes + decode_scratch_bytes,
    }


def _align_raw(payload_bytes: int) -> int:
    if payload_bytes < 0:
        raise ValueError("negative raw asset payload size")
    return (payload_bytes + RAW_ASSET_ALIGN_BYTES - 1) & ~(RAW_ASSET_ALIGN_BYTES - 1)


def full_metadata_bytes(kind_count, tables_bytes, entry_count, raw_count):
    """Bytes through data_off when the complete MFP2 metadata is retained."""
    pos = PACK_HEADER_BYTES + KIND_ROW_BYTES * kind_count
    pos = _align4(pos) + tables_bytes
    pos = _align4(pos) + PACK_ENTRY_BYTES * entry_count
    pos += 2 * entry_count  # by-id u16 indices
    pos = _align4(pos) + PACK_RAW_ENTRY_BYTES * raw_count
    return _align4(pos)


def mft1_arm_storage_bytes(blob: bytes) -> int:
    """Compute target NdsMf table expansion from a valid MFT1 shape."""
    if len(blob) < MFT1_HEADER_BYTES + 2 * MFT1_RECIP_COUNT:
        raise ValueError("truncated MFT1 blob")
    magic, nwords, nsucc, ntables, _reserved = struct.unpack_from("<IHHHH", blob, 0)
    if magic != MFT1_MAGIC or nwords == 0:
        raise ValueError("invalid MFT1 header")
    pos = MFT1_HEADER_BYTES + 2 * MFT1_RECIP_COUNT + 2 * nwords
    if pos > len(blob):
        raise ValueError("MFT1 word array exceeds blob")
    for _ in range(nsucc):
        if pos + 4 > len(blob):
            raise ValueError("truncated MFT1 successor header")
        _ctx, length = struct.unpack_from("<HH", blob, pos)
        pos += 4 + 2 * length
        if length == 0 or pos > len(blob):
            raise ValueError("invalid MFT1 successor span")
    total_syms = 0
    for _ in range(ntables):
        if pos + MFT1_TABLE_HEADER_BYTES > len(blob):
            raise ValueError("truncated MFT1 table header")
        _cls, _ctx, nsym, nesc = struct.unpack_from("<BBHH", blob, pos)
        pos += MFT1_TABLE_HEADER_BYTES + 2 * nsym + 2 * nesc
        if nsym == 0 or pos > len(blob):
            raise ValueError("invalid MFT1 table span")
        total_syms += nsym
    if pos != len(blob):
        raise ValueError("MFT1 parse ended at %d of %d" % (pos, len(blob)))
    return _align4(
        ARM_NDS_MF_TABLE_BYTES * ntables
        + 4 * total_syms
        + 4 * (nwords + 1)
    )


def _source_modules(corpus):
    """Load the existing O2R reader solely to size the 29 raw exceptions."""
    generator = mc._load_module(
        "mf_raw_exception_generator",
        mc.ROOT / "scripts" / "generate_battlepack_anim.py",
    )
    probe = mc._load_module(
        "mf_raw_exception_probe",
        mc.ROOT / "scripts" / "ftanim_reloc_probe.py",
    )
    generator.AOBJ32_IDS = set(corpus.get("aobj32", ())) | set(generator.AOBJ32_IDS)
    return generator, probe


def raw_exception_inventory(corpus):
    """Return every explicit raw exception with identity and both size bases.

    Unknown skip reasons fail closed: an unreadable/unbanked clip is not
    silently promoted into the raw exception list.
    """
    users = main_user_masks(corpus)
    generator, probe = _source_modules(corpus)
    rows = []
    seen = set()
    for filename, reason in corpus.get("skipped", ()):
        if reason.startswith("AObj32 "):
            raw_reason = RAW_AOBJ32
        elif reason.startswith("spline TraI "):
            raw_reason = RAW_SPLINE
        else:
            raise ValueError("unhandled MF corpus exclusion %s: %s" % (filename, reason))
        match = re.search(r"0x([0-9a-fA-F]+)", reason)
        if match is None:
            raise ValueError("raw exception has no asset id: %s (%s)" % (filename, reason))
        asset_id = int(match.group(1), 16)
        if asset_id in seen:
            raise ValueError("duplicate raw exception 0x%x" % asset_id)
        seen.add(asset_id)
        owner = mc.bank_of(asset_id)
        if owner is None:
            raise ValueError("raw exception 0x%x has no bank owner" % asset_id)
        path = mc.BANK / filename
        if not path.is_file():
            raise ValueError("raw exception source is absent: %s" % path)
        clip = generator.read_clip(probe, path)
        if clip is None or clip["asset_id"] != asset_id:
            raise ValueError("raw exception source no longer parses: %s" % filename)
        if raw_reason == RAW_AOBJ32 and not clip.get("aobj32"):
            raise ValueError("AObj32 exception changed type: %s" % filename)
        if raw_reason == RAW_SPLINE and not clip.get("spline"):
            raise ValueError("spline exception changed type: %s" % filename)
        source_bytes = path.read_bytes()
        rows.append({
            "asset_id": asset_id,
            "name": filename,
            "kind": owner,
            "kind_index": mc.KINDS.index(owner),
            "reason": raw_reason,
            "reason_name": "aobj32" if raw_reason == RAW_AOBJ32 else "spline",
            "main_user_mask": users.get(asset_id, 0),
            "payload_bytes": int(clip["payload_bytes"]),
            "source_bytes": len(source_bytes),
            "source_crc32": zlib.crc32(source_bytes) & 0xFFFFFFFF,
        })

    if len(rows) != EXPECTED_RAW_EXCEPTION_COUNT:
        raise ValueError("raw exception count %d != frozen %d" %
                         (len(rows), EXPECTED_RAW_EXCEPTION_COUNT))
    raw_aobj32 = {row["asset_id"] for row in rows if row["reason"] == RAW_AOBJ32}
    if raw_aobj32 != set(corpus.get("aobj32", ())):
        raise ValueError("raw AObj32 inventory differs from generated manifest ids")
    main_refs = set(users)
    missing_aobj32 = (set(corpus.get("aobj32", ())) & main_refs) - seen
    if missing_aobj32:
        raise ValueError("referenced AObj32 ids lack raw rows: %s" %
                         ", ".join("0x%x" % x for x in sorted(missing_aobj32)))
    return sorted(rows, key=lambda row: (row["kind_index"], row["asset_id"]))
