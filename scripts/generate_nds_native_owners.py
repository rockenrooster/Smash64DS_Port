#!/usr/bin/env python3
"""Generate the canonical Mario/Fox native-owner IR include.

The canonical bytes were recovered once from the exact profile-2 experiment
recorded by logs/native-fighter-cut/native-dead-state-profile2.json.  That ROM
was built from nds_renderer.o SHA256
00ada7d880c732a177d041b37b4642e95464af9c2dd73f70e3289287cd5565ba.
The old runtime executor is deliberately not recovered: it re-entered generic
state/triangle machinery and failed the big-jump performance gate.  These
hashed data sections retain only the source-derived owner IR needed by the new
direct renderer.
"""

from __future__ import annotations

import argparse
import base64
import hashlib
import struct
from pathlib import Path


EXPORT = {
    "state": (
        "ARAA4wCAAAACAAAABX4S/P/zF/8DAAAAAAEA9QAAAAUHAAAAAABQ9VBAAQcHAAAAAARA9VBBCQAHAAAAAAAA8ADAAwUIAAAAAgAA1xic//8EAAAAIAAI8nzAFwAKAAAAAABQ/fBlAAAGAAAAAAAA8wD0CwcJAAAAARAA4wAAAAACAAAABX4y/P/9F/8DAAAAAAAA1wAAAAAEAAAABf7//P99Fv8DAAAAAARA9VBDCQAHAAAAAAAQ/XBvAAAGAAAAAgAA1xKD//8EAAAAmwAn8hfBNgAKAAAAAAAA8wD0DwcJAAAAAABQ9UBAAQcHAAAAAAJA9UBDCQAHAAAAAgAA1///z9wEAAAAVUEP8rEBFwAKAAAAAABQ/XhsAAAGAAAAAAAA8wD4BQcJAAAABX4y/P/3F/8DAAAAAAAA+v8AKX0LAAAAAAAA+v+Z4f8LAAAAAgAA1/////8EAAAAAAAA8nzABwAKAAAAAABQ/dh5AAAGAAAAAAAA8wD4BwcJAAAA//v/2QAAAAAFAAAA////2QAEAAAFAAAAAgAA17ny//8EAAAAJoAD8qJACwAKAAAAAABQ/dB1AAAGAAAAAAAA+v8EYqgLAAAAAABQ9VAAAQcHAAAAAARA9VACCQAHAAAAAAAQ/Qh4AAAGAAAAAgAA1///H10EAAAAPOIX8niiHwAKAAAAAAAA8wD0BwcJAAAAAAAQ/dhyAAAGAAAAAgAA1///rWkEAAAAM8EH8q+BFwAKAAAAAAAA+v+q7u4LAAAAAABQ9TDAAAcHAAAAAAJA9TDCCAAHAAAAAAAQ/Qh1AAAGAAAAAAAA8hzAAQAKAAAAAABQ/eB6AAAGAAAAAAAA8wD4AQcJAAAA",
        "b332130988708066b956a2c43160ffe77c07c8281cbb7da380e2ad8492252045",
    ),
    "sequence": (
        "AAECAwQFBgcICQoLDAsMDQwAAQIDDg8FEBESExQFFRYXGAoLDBkaGwsMCwwNDAsMCwwNDAsMCwwNDAsMAAECExQFHB0eHwoLDCAhDQsAAQITFAUiIyQfCgsMDRklDA0MGSUMAAECJicoBSkqKwMOLAUtLhIKDQwZLyULDA0ZJQwNDAsMCwwLDA0LDAsMCwwNAAECMDEyBRwzNDUKAAECMDEyBRwzNDUK",
        "3feef24dc68c5a1600c196b4b57cb7402163ab02c439b3c16d758315928b2a2e",
    ),
    "vertex": (
        "ABUAGZgHAAAAAAAAAAgACSgJAAAAAAAAAQgAAAAAAAAAAAAAAQkBAAAAAAAAAAAAAQoCAAAAAAAAAAAAAQsDAAAAAAAAAAAAAAwEBLgJAAAAAAAAAAcAEPgJAAAAAAAAABUAE/gKAAAAAAAAADEABigMAAAAAAAAADkAFogMAAAAAAAAAEoAEOgNAAAAAAAAAFkAF+gOAAAAAAAAAAgACVgQAAAAAAAAAQgAAAAAAAAAAAAAAQkBAAAAAAAAAAAAAQoCAAAAAAAAAAAAAQsDAAAAAAAAAAAAAAwEBugQAAAAAAAAAAcAEEgRAAAAAAAAAAgAC0gSAAAAAAAAAQgAAAAAAAAAAAAAAQkBAAAAAAAAAAAAAQoCAAAAAAAAAAAAAQsDAAAAAAAAAAAAAQwEAAAAAAAAAAAAAA0FBvgSAAAAAAAAAAQAEFgTAAAAAAAAAAgAC1gUAAAAAAAAAQgAAAAAAAAAAAAAAQkBAAAAAAAAAAAAAQoCAAAAAAAAAAAAAQsDAAAAAAAAAAAAAQwEAAAAAAAAAAAAAA0FBggVAAAAAAAAAAQAEGgVAAAAAAAAAAgACJAIAAAAAAAAAB4ACRAJAAAAAAAAAAkABqAJAAAAAAAAAA0ABgAKAAAAAAAAABcABmAKAAAAAAAAACAAAmAKAAAAAAAAACECApAKAAAAAAAAACIEBcAKAAAAAAAAADkACRALAAAAAAAAAAgACaALAAAAAAAAABEABKALAAAAAAAAABIEBTAMAAAAAAAAAAgABYAMAAAAAAAAAAcACdAMAAAAAAAAAAgABWANAAAAAAAAABUACrANAAAAAAAAACkAClAOAAAAAAAAADYAFvAOAAAAAAAAAEsADVAQAAAAAAAAAFIAHyARAAAAAAAAAAgACRATAAAAAAAAABEAAhATAAAAAAAAABICAlATAAAAAAAAABMEBaATAAAAAAAAAAgABfATAAAAAAAAAAcACUAUAAAAAAAAAAgADNAUAAAAAAAAAAgAB5AVAAAAAAAAAAgABgAWAAAAAAAAABIABWAWAAAAAAAAAAgADLAWAAAAAAAAAAgAB3AXAAAAAAAAAAgABuAXAAAAAAAAABIABUAYAAAAAAAAABUACZAYAAAAAAAAARUAAAAAAABOApoAARYBAAAAAAC8AvQAARcCAAAAAABOApoAARgDAAAAAACcAQkAABkEBSAZAAAAAAAA",
        "0582f7e74a4649498a9f60e6518d0aa494416258cc42cb170520dd1db3a3d36b",
    ),
    "triangles": (
        "9uL4VtTid1ZW3tFSFMOPQhDXDlZy3rNVLNLLVdLG0Ckpsm4haJ0GHbHITxFIualEA5liGEShKRSJjGgQjL6APYmB6kEEvAkQaIjoDOOYwwChoAQVpJgBCaCEwBTkoMQcR4xnGEGcxQxlgAUE4ZAlEM29zDVsuW4pi6VJLei57zROmawV5KHkPGmoiQnImawIp7RiJKKEIgwFhOUABJxmBEaNJgAGoAQBMMoyPtDFrkWNxW0xTa1LJQql6iDMxKZEpJiGLcSMKwmBjGYNY4lhAEOA7SjNneVFg5SjCKKEIgCT1rJSs8aSQhK+bka1xVU2LrKNRfKt6yktsUshbaENJXK1Rh2nmKYQxJlCDUGMAwTDAc297DlvtY8tyrWqJW2hix3NpA0Z5bBkHWugpxBjkWIQgoQlEAUEtNp2UpXKsUoSxs09jr2rPcq1LD2KuS8tqq0qMSup5iComGIQgoQBCAOgAx0HmagY5JgFAYWY4xCDiKQEJIgFBAmMASRpoEMc45hHBCGVJxDNva4x67nKMcup7SXoreg8rJmsKEuVaBGmsAcNxYgkICiMqxBEhEUQoqXCNCC9KQgJhAEM4J3gDEKNIilIpegoKJlkKESdyBRJhCkYBpQmAKSAqBzkFEqMaiQojWgQR6UnIUKZiADChMcoB5UIFKGAwRSnGM297DltuW4pyanMJQupJyksnasZ6KjLIOyUxxQHmWIQQZAEBIOoSREqoUchQ53JIOWgiQCmoKcIR4zFBCWIKRgJBICoSRGJjEAhSJ0qHcOkQxgilMkcopimHKCEABUFHc29jTnLseo5K7nKJYuh6ySsmQsdzKDlIAWZqSjplGIQgoQBCMWc5BjljEQc44ihDAOEQwDmoAYV5JjEDGSIwxSjhGIEIoAFBIOUIAiDlCAIg5RFDGGIAgRoiAcJR4TmBAWd5RjFkAEQxATmoKgYpKBlGESMIxgChGQUYQjmoKYcBpEDHUOgJwynhAAJgKCGFKGQgQBioAIdaITmIEWcKBAFiAYRppyhEAGUhhRikCIMgoACBOagBR3kmOUQBo3ECAGVYSAGiGYAAYxFAIWIoQBikCIMgoACBAelJxmmnKQYxIxDEEGMIQFAhCYMCYEmBQelyRwloaQgI5WjCGmEASST1nRKs8YzQm/CMDoPto5B67VLPaulSC2poWglh8rHUKeYhxRElkcSzI3DCUOEIwAgsGwAarGKJQuxxRyGnGYQIAi8+35zXu9Zex371V710nRWctLRVrHCL0LUydI1jrVJLQul6CRLmSUdhaRmLIKUYRgBDOag5RjFkMMgxIxiIGSEQwADhEcgp4AHCEOgAx0GiQcVRoTIFIGYARCGlOAQYJzkFGKQIxABkEAQ5qAGFaSgBA2mkMcII5ADBIaEJggBiOMAA53gCEmtCSlLnWkZqaBEIcmU5CjrjAQVS5hrCOOEhwSkgGIEBIQGFEaAQASkmIUMg4gkCAOUYAgFiKEIg5RiFCSUQBQBFGKQQRAEBEmtKC0qncosBa0kIaas5CTqjAQVxYhhHOGQpABFgIEAAYhKGGqIQQykmMMURYzBDKGAARhhiEEUg5RDEKOEIwADCGKQggQBCGiIYCBInEcE6JgAFSeQ5hDIlIYUAZShEGKgAh3moAYVBY2GHOKQZQCChAQEppCkAA==",
        "b9e792e1730d8fb1e170eb4b7aad1070eb1bc762f9be31ea02aed66f41901246",
    ),
    "runs": (
        "AAAkAP///wEkAAwA/wEAADAACAH/AAAAOAAcAP//AABUABgA//8HAGwABAA/AAAAcAAZAP//PwCJABMA//8AAJwAFAD//38AsAAMAP8BAAC8AAgB/wMAAMQAHAD//wAA4AAPAP8HAADvAAQBHAcAAPMAAgCABwAA9QADAVcFAAD4AAIA4AUAAPoAAwFjAQAA/QABAOAAAAD+ABIA//8AABABDwD/BwAAHwEEARkHAAAjAQIAgAcAACUBAwFuAgAAKAEBAMACAAApAQEBZAAAACoBAQDgAAAAKwECASMBAAAtAQEAoAEAAC4BEgD//wAAQAEIAP8AAABIAQoA/wEAAFIBAgA/AAAAVAECAD8AAABWAQQAPwAAAFoBCQD/AQAAYwEJAP8BAABsAQwA/wEAAHgBDAD/AQAAhAEEAB8AAACIAQ4A/wEAAJYBBAAfAAAAmgEMAP8DAACmAQgA/wMAAK4BGgD//z8AyAEHAP8fAADPARkA////f+gBDAD/AQAA9AEMAP8BAAAAAgQAHwAAAAQCDgD/AQAAEgIUAP8PAAAmAggAfwAAAC4CBQA/AAAAMwIDAB8AAAA2AhQA/w8AAEoCCAB/AAAAUgIFAD8AAABXAgMAHwAAAFoCDAD/AQAAZgICAYwBAABoAgIA4AEAAGoCAQEoAQAAawIBANAAAABsAgQBvwAAAHACAQBwAAAAcQIBATEAAAA=",
        "ba135565be9a942bae556f12fbf221e56d6feebb71e172472cfc19805803486c",
    ),
    "epochs": (
        "AAAFAAAAAAAFBQIEAQEAFgsADAABAAEAAQEBAAEBAAkNAA4AAgACAAEBAQAFAQANDwD//wcAAwACAAEAAQH/CBEAGgAIAAQACQEEAgEBABYbAB0ACQAFAAIFAQQBAQEyIgAkAAoABgACAQEAAQECOiUA//8LAAcAAgABAAEB/0snAP//DAAIAAEAAQABAf9aKAApAA0ACQABAQEAAQEACSoAKwAOAAoAAQEBAAUBAA0sAP//EwALAAIAAQABAf8ILgAvABQADAABAQEAAQEACTAAMQAVAA0AAQEBAAYGAA4yADMAGwATAAEBAQABAQAFNAA1ABwAFAABAQEAAQEACTYANwAdABUAAQEBAAYIAA44ADkAIwAdAAEBAQABAQAFOgA7ACQAHgABAQEAAQEBCTwAQQAlAB8ABQUCBAEBAB9HAEgAJgAgAAECAQABAQMKSgD//ycAIQAAAAEAAQECDkoASgAoACIAAAIBAAEBARhMAP//KQAjAAEAAQADAf8jTQBSACwAJAAFBQIEAQEAOlgAWQAtACUAAQEBAAEBAAlaAP//LgAmAAEAAQACAf8TWwD//zAAJwADAAEAAQH/CV4A//8xACgAAgABAAEB/whgAP//MgApAAMAAQABAf8JYwBsADMAKgAJAQQCAQEBFm0AcwA0ACsABgEDAgEBACp0AP//NQAsAAMAAQABAf83dwD//zYALQACAAEAAQH/THkA//83AC4AAQABAAEB/1N6AHsAOAAvAAEBAQABAQAJfAD//zkAMAABAAEAAwH/FH0A//88ADEAAwABAAEB/wmAAP//PQAyAAIAAQABAf8IggCDAD4AMwABAQEAAQEACYQAhQA/ADQAAQEBAAEBAAmGAIcAQAA1AAEBAQABAQAJiAD//0EANgABAAEAAQH/E4kAigBCADcAAQEBAAEBAAmLAIwAQwA4AAEBAQABAQAJjQCOAEQAOQABAQEAAQEACY8A//9FADoAAQABAAEB/xOQAP//RgA7AAsABgABAf8WnAD//0cAPAALAAYABQf/Gg==",
        "7f0fa1a3dba3660c899e7c3184ef5f62bddec6b38d96ad4cbd9966f7946ce9e5",
    ),
    "mario_roots": (
        "aBYAAAAACgAsAAEBAgAAAMgXAAABAP//EAABAAAAAABIGAAAAgD//xIAAQAAAAAA2BgAAAMA//8XAAEAAAAAAJAZAAAEAP//ZQAFAAAAAAC4HAAACQD//xAAAQAAAAAAOB0AAAoA//8SAAEAAAAAAMgdAAALAP//FwABAAAAAACAHgAADAD//xIAAQAAAAAAEB8AAA0A//8XAAEAAAAAAMgfAAAOAP//DwABAAAAAABAIAAADwD//xIAAQAAAAAA0CAAABAA//8XAAEAAAAAAIghAAARAP//DwABAAAAAAA=",
        "bbc51381b9baa20e09b6a14e12e3ef6d9d253b7845cd2b4718b5148d5e38cac2",
    ),
    "fox_roots": (
        "cBkAABIARgAoAAIBAgAAALAaAAAUAFcAQwAFAQIAAADIHAAAGQD//xoAAgAAAAAAmB0AABsA//8MAAEAAAAAAPgdAAAcAP//EAABAAAAAAB4HgAAHQD//wwAAQAAAAAA2B4AAB4A//9hAAUAAAAAAOAhAAAjAP//GwACAAAAAAC4IgAAJQD//wwAAQAAAAAAGCMAACYA//8QAAEAAAAAAJgjAAAnAP//FAABAAAAAAA4JAAAKAD//w4AAQAAAAAAqCQAACkA//8WAAIAAAAAAFglAAArAP//FAABAAAAAAD4JQAALAD//w4AAQAAAAAAaCYAAC0A//8WAAIAAAAAABgnAAAvAJsAIAABAQIAAAAYKAAAMACnACQAAQECAAAA",
        "2f82f144939c5c952f79b8961593b84e7c2d6484a92bee07b84c9014d395a7c7",
    ),
}

O2R_ASSETS = {
    "mario": (
        Path("decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/MarioModel"),
        0x0128,
        "be1c3b6f909b42da2a973e2fe1977cd72c046d2447f0c4afa97fe8cd5429854f",
    ),
    "fox": (
        Path("decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/FoxModel"),
        0x0139,
        "8c49ded8144d153b25101afc6c71f5e455ef43e2ddc5326c0f71a72ae740b5a5",
    ),
}

# These are the primary JointTree DObjDesc arrays in the exact hashed O2R
# resources above.  BattleShip's source declarations are
# dMarioModel_JointTree (file offset 0x2200) and dFoxModel_JointTree (0x2938).
# Mario contains 25 raw descriptors and Fox contains 27, followed by the
# depth-18 sentinel. BattleShip ftmanager.c creates a separate synthetic TopN
# root, then lbCommonSetupFighterPartsDObjs applies the character setup mask:
# Mario selects raw descriptors 0..23 and Fox selects 0..25. Thus both live
# trees retain the raw cardinality after adding TopN and dropping the final
# unselected raw leaf. (The Mario source comment says 28 entries, but the exact
# hashed initializer/O2R payload reaches its sentinel at descriptor 25.)
OWNER_JOINT_TREES = {
    "mario": (0x2200, 26),
    "fox": (0x2938, 28),
}

# dMarioMain_setup_parts / dFoxMain_setup_parts, consumed MSB-first by
# BattleShip lbCommonSetupFighterPartsDObjs.
OWNER_SETUP_PARTS = {
    "mario": (0xffffff00, 0x00000000),
    "fox": (0xffffffc0, 0x00000000),
}

# Slot zero stays available for the camera/base modelview. Only bindings
# observed in canonical cross-matrix runs receive an owner-local GX palette
# slot. A packed corner uses slot 31 as the logical current-root slot; after an
# alternate-binding corner the emitter restores that root through its real
# per-binding palette slot.
OWNER_CROSS_BINDINGS = {
    "mario": (1, 2, 5, 6, 8, 9, 11, 12),
    "fox": (16, 17),
}

DIRECT_POLICY_CULL_NONE = 0x80
DIRECT_POLICY_FAMILIES = (
    # combine_w0, combine_w1, vertex-context flags, textured
    (0xfc127e05, 0xff17f3ff, "VERTEX|TEXTURE", 1),
    (0xfc327e05, 0xff17fdff, "MATERIAL|VERTEX", 0),
    (0xfcfffe05, 0xff167dff, "VERTEX", 0),
    (0xfc327e05, 0xff17f7ff, "MATERIAL|VERTEX", 0),
)
DIRECT_POLICY_TEXTURED_EPOCHS = frozenset((0, 4, 5, 19, 24, 30, 31, 47, 48))
DIRECT_POLICY_LIT_ONLY_EPOCHS = frozenset(
    (3, 11, 14, 17, 22, 26, 28, 32, 36, 38, 42, 46)
)
DIRECT_POLICY_ALT_ALPHA_EPOCHS = frozenset((7, 8, 27, 29, 33, 34, 37))
DIRECT_POLICY_CULL_NONE_EPOCHS = frozenset((20, 21))

O2R_RESOURCE_HEADER_SIZE = 0x40
SOURCE_VERTEX_SIZE = 16
VERTEX_CACHE_SIZE = 32
INVALID_DENSE_VERTEX = 0xffff
INVALID_U8 = 0xff
PACKED_DENSE_ID_BITS = 10
PACKED_DENSE_ID_LIMIT = 1 << PACKED_DENSE_ID_BITS
PACKED_GX_SLOT_CURRENT = 31
DOBJ_DESC_SIZE = 44

def decode_export() -> dict[str, bytes]:
    decoded: dict[str, bytes] = {}
    for name, (encoded, expected_hash) in EXPORT.items():
        payload = base64.b64decode(encoded, validate=True)
        actual_hash = hashlib.sha256(payload).hexdigest()
        if actual_hash != expected_hash:
            raise ValueError(f"{name}: SHA256 {actual_hash} != {expected_hash}")
        decoded[name] = payload
    return decoded


def unpack_many(fmt: str, payload: bytes):
    size = struct.calcsize(fmt)
    if len(payload) % size:
        raise ValueError(f"{len(payload)} bytes is not a multiple of {size}")
    return [item for item in struct.iter_unpack(fmt, payload)]


def load_o2r_payload(repo_root: Path, owner_name: str) -> bytes:
    relative_path, expected_file_id, expected_hash = O2R_ASSETS[owner_name]
    path = repo_root / relative_path
    source = path.read_bytes()
    actual_hash = hashlib.sha256(source).hexdigest()
    if actual_hash != expected_hash:
        raise ValueError(
            f"{owner_name} O2R: SHA256 {actual_hash} != {expected_hash}"
        )
    if len(source) < O2R_RESOURCE_HEADER_SIZE + 16:
        raise ValueError(f"{owner_name} O2R: truncated resource header")
    if source[4:8] != b"OLER":
        raise ValueError(f"{owner_name} O2R: invalid resource magic")
    file_id = struct.unpack_from("<I", source, O2R_RESOURCE_HEADER_SIZE)[0]
    if file_id != expected_file_id:
        raise ValueError(
            f"{owner_name} O2R: file ID 0x{file_id:x} != 0x{expected_file_id:x}"
        )
    extern_count = struct.unpack_from(
        "<I", source, O2R_RESOURCE_HEADER_SIZE + 8
    )[0]
    data_size_offset = O2R_RESOURCE_HEADER_SIZE + 12 + extern_count * 2
    if data_size_offset + 4 > len(source):
        raise ValueError(f"{owner_name} O2R: truncated extern table")
    data_size = struct.unpack_from("<I", source, data_size_offset)[0]
    data_offset = data_size_offset + 4
    data_end = data_offset + data_size
    if data_end != len(source):
        raise ValueError(
            f"{owner_name} O2R: data ends at 0x{data_end:x}, "
            f"file ends at 0x{len(source):x}"
        )
    return source[data_offset:data_end]


def decode_source_vertex(payload: bytes, source_offset: int):
    if source_offset < 0 or source_offset + SOURCE_VERTEX_SIZE > len(payload):
        raise ValueError(f"source vertex offset 0x{source_offset:x} is out of range")
    x, y, z, _, s, t, r, g, b, a = struct.unpack_from(
        ">hhhHhhBBBB", payload, source_offset
    )
    if a == 0:
        a = 0xff
    rgba = (r << 24) | (g << 16) | (b << 8) | a
    return x, y, z, s, t, rgba


def build_direct_epoch_policies(epoch_count: int) -> list[int]:
    if epoch_count != 49:
        raise ValueError(f"direct policy expects 49 epochs, got {epoch_count}")
    classified = (
        DIRECT_POLICY_TEXTURED_EPOCHS |
        DIRECT_POLICY_LIT_ONLY_EPOCHS |
        DIRECT_POLICY_ALT_ALPHA_EPOCHS
    )
    if max(classified | DIRECT_POLICY_CULL_NONE_EPOCHS) >= epoch_count:
        raise ValueError("direct policy names an out-of-range epoch")
    if ((DIRECT_POLICY_TEXTURED_EPOCHS & DIRECT_POLICY_LIT_ONLY_EPOCHS) or
            (DIRECT_POLICY_TEXTURED_EPOCHS &
             DIRECT_POLICY_ALT_ALPHA_EPOCHS) or
            (DIRECT_POLICY_LIT_ONLY_EPOCHS &
             DIRECT_POLICY_ALT_ALPHA_EPOCHS)):
        raise ValueError("direct policy families overlap")

    result = []
    for epoch_index in range(epoch_count):
        if epoch_index in DIRECT_POLICY_TEXTURED_EPOCHS:
            family = 0
        elif epoch_index in DIRECT_POLICY_LIT_ONLY_EPOCHS:
            family = 2
        elif epoch_index in DIRECT_POLICY_ALT_ALPHA_EPOCHS:
            family = 3
        else:
            family = 1
        if epoch_index in DIRECT_POLICY_CULL_NONE_EPOCHS:
            family |= DIRECT_POLICY_CULL_NONE
        result.append(family)
    return result


def decode_joint_topology(
        payload: bytes, owner_name: str, roots: list[tuple]):
    joint_tree_offset, descriptor_count = OWNER_JOINT_TREES[owner_name]
    tree_end = joint_tree_offset + descriptor_count * DOBJ_DESC_SIZE
    if tree_end > len(payload):
        raise ValueError(f"{owner_name} JointTree is out of range")

    depths = []
    display_offsets = []
    for descriptor_index in range(descriptor_count):
        descriptor_offset = joint_tree_offset + descriptor_index * DOBJ_DESC_SIZE
        depth, reloc_pointer = struct.unpack_from(
            ">II", payload, descriptor_offset
        )
        if reloc_pointer == 0:
            display_offset = None
        else:
            # O2R internal reloc words carry the next relocation word in the
            # high half and the pointer target word offset in the low half.
            display_offset = (reloc_pointer & 0xffff) * 4
            if display_offset >= len(payload):
                raise ValueError(
                    f"{owner_name} JointTree entry {descriptor_index}: "
                    f"display target 0x{display_offset:x} is out of range"
                )
        depths.append(depth)
        display_offsets.append(display_offset)

    if ((depths[-1] != 18) or (display_offsets[-1] is not None)):
        raise ValueError(f"{owner_name} JointTree lost its depth-18 sentinel")
    depths = depths[:-1]
    display_offsets = display_offsets[:-1]
    raw_descriptor_count = descriptor_count - 1
    if (len(depths) != raw_descriptor_count) or (depths[0] != 0):
        raise ValueError(f"{owner_name} JointTree topology cardinality changed")

    setup_words = OWNER_SETUP_PARTS[owner_name]
    selected_indices = []
    for descriptor_index in range(raw_descriptor_count):
        word_index = descriptor_index // 32
        bit_index = 31 - (descriptor_index & 31)
        if setup_words[word_index] & (1 << bit_index):
            selected_indices.append(descriptor_index)
    if selected_indices != list(range(len(selected_indices))):
        raise ValueError(
            f"{owner_name} setup_parts is no longer one contiguous prefix"
        )
    selected_count = len(selected_indices)
    if selected_count + 1 != raw_descriptor_count:
        raise ValueError(
            f"{owner_name} synthetic-TopN live cardinality changed"
        )
    if any(offset is not None for offset in display_offsets[selected_count:]):
        raise ValueError(
            f"{owner_name} setup_parts dropped a drawable raw descriptor"
        )
    depths = depths[:selected_count]
    display_offsets = display_offsets[:selected_count]

    root_offsets = [root[0] for root in roots]
    selected_display_offsets = [
        offset for offset in display_offsets if offset is not None
    ]
    if selected_display_offsets != root_offsets:
        raise ValueError(
            f"{owner_name} JointTree display preorder does not match "
            "canonical native roots"
        )
    root_by_offset = {
        offset: binding for binding, offset in enumerate(root_offsets)
    }

    raw_parents = []
    raw_bindings = []
    depth_stack = []
    for joint_index, (depth, display_offset) in enumerate(
            zip(depths, display_offsets)):
        if depth >= 18:
            raise ValueError(
                f"{owner_name} joint {joint_index}: invalid live depth {depth}"
            )
        if joint_index == 0:
            if depth != 0:
                raise ValueError(f"{owner_name} JointTree has no depth-zero root")
            parent = INVALID_U8
        else:
            if (depth == 0) or (depth > (depths[joint_index - 1] + 1)):
                raise ValueError(
                    f"{owner_name} joint {joint_index}: discontinuous depth {depth}"
                )
            if depth - 1 >= len(depth_stack):
                raise ValueError(
                    f"{owner_name} joint {joint_index}: missing parent at "
                    f"depth {depth - 1}"
                )
            parent = depth_stack[depth - 1]
        if len(depth_stack) <= depth:
            depth_stack.append(joint_index)
        else:
            depth_stack[depth] = joint_index
            del depth_stack[depth + 1:]
        raw_parents.append(parent)
        raw_bindings.append(
            INVALID_U8 if display_offset is None else
            root_by_offset[display_offset]
        )

    # ftManagerMakeFighter creates TopN before the selected JointTree prefix.
    # Shift every raw index by one and parent raw roots directly to TopN.
    parents = [INVALID_U8]
    bindings = [INVALID_U8]
    parents.extend(
        0 if parent == INVALID_U8 else parent + 1
        for parent in raw_parents
    )
    bindings.extend(raw_bindings)

    binding_joints = [INVALID_U8] * len(roots)
    for joint_index, binding in enumerate(bindings):
        if binding != INVALID_U8:
            if binding_joints[binding] != INVALID_U8:
                raise ValueError(
                    f"{owner_name} binding {binding} appears more than once"
                )
            binding_joints[binding] = joint_index
    if any(joint == INVALID_U8 for joint in binding_joints):
        raise ValueError(f"{owner_name} JointTree does not cover every binding")

    binding_parents = []
    for binding, joint_index in enumerate(binding_joints):
        ancestor = parents[joint_index]
        while ((ancestor != INVALID_U8) and
               (bindings[ancestor] == INVALID_U8)):
            ancestor = parents[ancestor]
        binding_parents.append(
            INVALID_U8 if ancestor == INVALID_U8 else bindings[ancestor]
        )

    cross_slots = [PACKED_GX_SLOT_CURRENT] * len(roots)
    for palette_slot, binding in enumerate(
            OWNER_CROSS_BINDINGS[owner_name], start=1):
        if binding >= len(roots):
            raise ValueError(
                f"{owner_name} cross binding {binding} is out of range"
            )
        cross_slots[binding] = palette_slot

    joint_schedule = []
    for joint_index, (parent, binding) in enumerate(zip(parents, bindings)):
        packed_parent = 31 if parent == INVALID_U8 else parent
        packed_binding = 31 if binding == INVALID_U8 else binding
        palette_slot = (
            31 if binding == INVALID_U8 else cross_slots[binding]
        )
        if ((packed_parent > 31) or (packed_binding > 31) or
                (palette_slot > 31)):
            raise ValueError(
                f"{owner_name} joint {joint_index}: topology exceeds packed ABI"
            )
        joint_schedule.append(
            packed_parent | (packed_binding << 5) | (palette_slot << 10)
        )
    return joint_schedule, binding_parents, binding_joints, cross_slots


def build_dense_geometry(
        vertex, triangles, runs, epochs, owners, repo_root: Path | None = None):
    if repo_root is None:
        repo_root = Path(__file__).resolve().parents[1]
    repo_root = Path(repo_root).resolve()
    payloads = {
        owner_name: load_o2r_payload(repo_root, owner_name)
        for owner_name, _ in owners
    }
    dense_vertices = []
    dense_color_sources = []
    dense_owners = []
    dense_corners = []
    action_dense_first = [INVALID_DENSE_VERTEX] * len(vertex)
    run_first_corner = [INVALID_DENSE_VERTEX] * len(runs)
    run_owners = []
    run_root_bindings = []
    run_binding_sets = []
    next_epoch = 0
    next_action = 0
    next_run = 0

    for owner_index, (owner_name, roots) in enumerate(owners):
        payload = payloads[owner_name]
        slots = [None] * VERTEX_CACHE_SIZE
        for root_ordinal, root in enumerate(roots):
            first_epoch = root[1]
            epoch_count = root[4]
            if first_epoch != next_epoch:
                raise ValueError(
                    f"{owner_name} root {root_ordinal}: epoch {first_epoch} "
                    f"is not source-order epoch {next_epoch}"
                )
            for epoch_index in range(first_epoch, first_epoch + epoch_count):
                if epoch_index >= len(epochs):
                    raise ValueError(f"epoch {epoch_index} is out of range")
                epoch = epochs[epoch_index]
                first_action = epoch[2]
                first_run = epoch[3]
                action_count = epoch[8]
                run_count = epoch[9]
                if first_action != next_action:
                    raise ValueError(
                        f"epoch {epoch_index}: action {first_action} is not "
                        f"source-order action {next_action}"
                    )
                if first_run != next_run:
                    raise ValueError(
                        f"epoch {epoch_index}: run {first_run} is not "
                        f"source-order run {next_run}"
                    )

                for action_index in range(
                        first_action, first_action + action_count):
                    if action_index >= len(vertex):
                        raise ValueError(f"vertex action {action_index} is out of range")
                    kind, _, index, count, source_offset, s, t = vertex[action_index]
                    if index >= VERTEX_CACHE_SIZE:
                        raise ValueError(
                            f"vertex action {action_index}: slot {index} is out of range"
                        )
                    if kind == 0:
                        if index + count > VERTEX_CACHE_SIZE:
                            raise ValueError(
                                f"vertex action {action_index}: slots "
                                f"{index}..{index + count - 1} are out of range"
                            )
                        if source_offset + count * SOURCE_VERTEX_SIZE > len(payload):
                            raise ValueError(
                                f"vertex action {action_index}: source block is out of range"
                            )
                        if count:
                            action_dense_first[action_index] = len(dense_vertices)
                        for block_index in range(count):
                            decoded = decode_source_vertex(
                                payload,
                                source_offset + block_index * SOURCE_VERTEX_SIZE,
                            )
                            dense_id = len(dense_vertices)
                            dense_vertices.append(
                                (
                                    *decoded[:5],
                                    root_ordinal,
                                    index + block_index,
                                    decoded[5],
                                )
                            )
                            dense_color_sources.append(dense_id)
                            dense_owners.append(owner_index)
                            slots[index + block_index] = dense_id
                    elif kind == 1:
                        previous_id = slots[index]
                        if previous_id is None:
                            raise ValueError(
                                f"vertex action {action_index}: MODIFY_ST slot "
                                f"{index} is not live"
                            )
                        x, y, z, _, _, binding, cache_slot, rgba = (
                            dense_vertices[previous_id]
                        )
                        if cache_slot != index:
                            raise ValueError(
                                f"vertex action {action_index}: MODIFY_ST slot "
                                f"{index} aliases source slot {cache_slot}"
                            )
                        dense_id = len(dense_vertices)
                        action_dense_first[action_index] = dense_id
                        dense_vertices.append(
                            (x, y, z, s, t, binding, index, rgba)
                        )
                        dense_color_sources.append(
                            dense_color_sources[previous_id]
                        )
                        dense_owners.append(owner_index)
                        slots[index] = dense_id
                    else:
                        raise ValueError(
                            f"vertex action {action_index}: unsupported kind {kind}"
                        )
                    next_action += 1

                for run_index in range(first_run, first_run + run_count):
                    if run_index >= len(runs):
                        raise ValueError(f"run {run_index} is out of range")
                    first_triangle, triangle_count, submit_class, required_mask = (
                        runs[run_index]
                    )
                    if first_triangle + triangle_count > len(triangles):
                        raise ValueError(f"run {run_index}: triangle range is out of bounds")
                    run_first_corner[run_index] = len(dense_corners)
                    actual_mask = 0
                    bindings = set()
                    for triangle_index in range(
                            first_triangle, first_triangle + triangle_count):
                        compact = triangles[triangle_index] & 0x7fff
                        triangle_slots = (
                            (compact >> 10) & 31,
                            (compact >> 5) & 31,
                            compact & 31,
                        )
                        for slot in triangle_slots:
                            actual_mask |= 1 << slot
                            dense_id = slots[slot]
                            if dense_id is None:
                                raise ValueError(
                                    f"run {run_index}: triangle {triangle_index} "
                                    f"uses non-live slot {slot}"
                                )
                            dense_corners.append(dense_id)
                            if dense_vertices[dense_id][6] != slot:
                                raise ValueError(
                                    f"run {run_index}: dense vertex {dense_id} "
                                    f"records slot {dense_vertices[dense_id][6]}, "
                                    f"used through slot {slot}"
                                )
                            bindings.add(dense_vertices[dense_id][5])
                    if actual_mask != required_mask:
                        raise ValueError(
                            f"run {run_index}: slot mask 0x{actual_mask:08x} != "
                            f"0x{required_mask:08x}"
                        )
                    if submit_class == 0:
                        if bindings != {root_ordinal}:
                            raise ValueError(
                                f"raw run {run_index}: bindings {sorted(bindings)} "
                                f"do not match root {root_ordinal}"
                            )
                    elif submit_class == 1:
                        if root_ordinal not in bindings or len(bindings) < 2:
                            raise ValueError(
                                f"cross run {run_index}: bindings "
                                f"{sorted(bindings)} do not preserve root crossing"
                            )
                    else:
                        raise ValueError(
                            f"run {run_index}: unsupported submit class {submit_class}"
                        )
                    run_owners.append(owner_index)
                    run_root_bindings.append(root_ordinal)
                    run_binding_sets.append(frozenset(bindings))
                    next_run += 1
                next_epoch += 1

    if (next_epoch, next_action, next_run) != (
            len(epochs), len(vertex), len(runs)):
        raise ValueError(
            "owner traversal did not consume every epoch, vertex action, and run"
        )
    if len(dense_vertices) > INVALID_DENSE_VERTEX:
        raise ValueError("dense vertex IDs exceed the u16 encoding")
    if len(dense_corners) != len(triangles) * 3:
        raise ValueError(
            f"dense corner count {len(dense_corners)} != {len(triangles) * 3}"
        )
    if any(value == INVALID_DENSE_VERTEX for value in action_dense_first):
        raise ValueError("a canonical vertex action created no dense record")
    if any(value == INVALID_DENSE_VERTEX for value in run_first_corner):
        raise ValueError("a canonical run has no dense first-corner offset")
    if not (len(dense_vertices) == len(dense_color_sources) ==
            len(dense_owners)):
        raise ValueError("dense metadata cardinality mismatch")
    if not (len(runs) == len(run_owners) == len(run_root_bindings) ==
            len(run_binding_sets)):
        raise ValueError("run metadata cardinality mismatch")
    return (
        dense_vertices,
        dense_color_sources,
        dense_owners,
        dense_corners,
        action_dense_first,
        run_first_corner,
        run_owners,
        run_root_bindings,
        run_binding_sets,
    )


def build_direct_dense_tables(
        vertex, runs, dense_vertices, dense_color_sources, dense_corners,
        action_dense_first, run_first_corner, run_owners,
        run_root_bindings, run_binding_sets, owner_cross_slots):
    if len(dense_vertices) >= PACKED_DENSE_ID_LIMIT:
        raise ValueError(
            f"{len(dense_vertices)} dense IDs exceed the 10-bit direct ABI"
        )
    action_dense_spans = []
    for action_index, action in enumerate(vertex):
        kind, _, _, count, _, _, _ = action
        dense_count = count if kind == 0 else 1
        dense_first = action_dense_first[action_index]
        if ((dense_count == 0) or (dense_count > 31) or
                (dense_first >= PACKED_DENSE_ID_LIMIT) or
                ((dense_first + dense_count) > len(dense_vertices))):
            raise ValueError(
                f"vertex action {action_index}: dense span does not fit "
                "the packed direct ABI"
            )
        action_dense_spans.append(dense_first | (dense_count << 10))

    packed_corners = []
    run_first_unique = []
    run_unique_count = []
    run_unique_dense = []
    observed_cross_bindings = [set() for _ in owner_cross_slots]
    for run_index, run in enumerate(runs):
        _, triangle_count, submit_class, _ = run
        corner_first = run_first_corner[run_index]
        corner_count = triangle_count * 3
        if corner_first != len(packed_corners):
            raise ValueError(
                f"run {run_index}: dense corners are not source ordered"
            )
        owner_index = run_owners[run_index]
        root_binding = run_root_bindings[run_index]
        cross_slots = owner_cross_slots[owner_index]
        if submit_class == 1:
            observed_cross_bindings[owner_index].update(
                run_binding_sets[run_index]
            )
            if ((root_binding >= len(cross_slots)) or
                    (cross_slots[root_binding] == PACKED_GX_SLOT_CURRENT)):
                raise ValueError(
                    f"cross run {run_index}: current binding {root_binding} "
                    "has no restorable GX palette slot"
                )

        unique = []
        seen = set()
        for dense_id in dense_corners[
                corner_first:corner_first + corner_count]:
            if dense_id >= PACKED_DENSE_ID_LIMIT:
                raise ValueError(
                    f"run {run_index}: dense ID {dense_id} is not packable"
                )
            binding = dense_vertices[dense_id][5]
            if submit_class == 0:
                if binding != root_binding:
                    raise ValueError(
                        f"raw run {run_index}: non-current binding {binding}"
                    )
                palette_slot = PACKED_GX_SLOT_CURRENT
            else:
                if binding == root_binding:
                    palette_slot = PACKED_GX_SLOT_CURRENT
                else:
                    if binding >= len(cross_slots):
                        raise ValueError(
                            f"cross run {run_index}: binding {binding} is out "
                            "of range"
                        )
                    palette_slot = cross_slots[binding]
                    if palette_slot == PACKED_GX_SLOT_CURRENT:
                        raise ValueError(
                            f"cross run {run_index}: binding {binding} has no "
                            "GX palette slot"
                        )
            packed_corners.append(dense_id | (palette_slot << 10))
            if dense_id not in seen:
                seen.add(dense_id)
                unique.append(dense_id)

        if len(run_unique_dense) > 0xffff:
            raise ValueError("direct run unique list exceeds its u16 index ABI")
        if len(unique) > 0xff:
            raise ValueError(
                f"run {run_index}: {len(unique)} unique dense IDs exceed u8"
            )
        run_first_unique.append(len(run_unique_dense))
        run_unique_count.append(len(unique))
        run_unique_dense.extend(unique)

    if len(packed_corners) != len(dense_corners):
        raise ValueError("packed direct corner cardinality mismatch")
    for owner_index, (owner_name, _) in enumerate(O2R_ASSETS.items()):
        expected = set(OWNER_CROSS_BINDINGS[owner_name])
        if observed_cross_bindings[owner_index] != expected:
            raise ValueError(
                f"{owner_name} cross-binding census changed: "
                f"{sorted(observed_cross_bindings[owner_index])}"
            )
    for dense_id, color_source in enumerate(dense_color_sources):
        if ((color_source > dense_id) or
                (color_source >= PACKED_DENSE_ID_LIMIT)):
            raise ValueError(
                f"dense vertex {dense_id}: invalid color source {color_source}"
            )

    return (
        action_dense_spans,
        packed_corners,
        run_first_unique,
        run_unique_count,
        run_unique_dense,
    )


def emit_rows(type_name: str, name: str, rows: list[str]) -> list[str]:
    result = [f"static const {type_name} {name}[{len(rows)}] =", "{"]
    result.extend(f"    {row}," for row in rows)
    result.append("};")
    result.append("")
    return result


def generate(repo_root: Path | None = None) -> str:
    if repo_root is None:
        repo_root = Path(__file__).resolve().parents[1]
    repo_root = Path(repo_root).resolve()
    data = decode_export()
    state = unpack_many("<IIB3x", data["state"])
    sequence = list(data["sequence"])
    vertex = unpack_many("<BBBBIhh", data["vertex"])
    triangles = [item[0] for item in unpack_many("<H", data["triangles"])]
    runs = unpack_many("<HBBI", data["runs"])
    epochs = unpack_many("<HHHHBBBBBBBB", data["epochs"])
    mario_roots = unpack_many("<IHHHBBBB2x", data["mario_roots"])
    fox_roots = unpack_many("<IHHHBBBB2x", data["fox_roots"])
    if (len(state), len(sequence), len(vertex), len(triangles), len(runs),
            len(epochs), len(mario_roots), len(fox_roots)) != (
            54, 168, 76, 626, 67, 49, 14, 18):
        raise ValueError("canonical native-fighter IR cardinality changed")
    class_triangles = [0, 0]
    for _, triangle_count, submit_class, _ in runs:
        if submit_class >= len(class_triangles):
            raise ValueError(f"unsupported submit class {submit_class}")
        class_triangles[submit_class] += triangle_count
    if class_triangles != [582, 44]:
        raise ValueError(f"submit-class census changed: {class_triangles}")
    direct_epoch_policies = build_direct_epoch_policies(len(epochs))
    owner_roots = (("mario", mario_roots), ("fox", fox_roots))
    owner_topologies = []
    for owner_name, roots in owner_roots:
        payload = load_o2r_payload(repo_root, owner_name)
        owner_topologies.append(
            decode_joint_topology(payload, owner_name, roots)
        )
    (
        dense_vertices,
        dense_color_sources,
        dense_owners,
        dense_corners,
        action_dense_first,
        run_first_corner,
        run_owners,
        run_root_bindings,
        run_binding_sets,
    ) = build_dense_geometry(
        vertex,
        triangles,
        runs,
        epochs,
        owner_roots,
        repo_root,
    )
    if (len(dense_vertices), len(dense_corners)) != (541, 1878):
        raise ValueError(
            "canonical dense fighter geometry cardinality changed: "
            f"{len(dense_vertices)} vertices, {len(dense_corners)} corners"
        )
    if dense_owners.count(0) != 255 or dense_owners.count(1) != 286:
        raise ValueError("canonical owner dense-vertex census changed")
    owner_cross_slots = [topology[3] for topology in owner_topologies]
    (
        action_dense_spans,
        packed_corners,
        run_first_unique,
        run_unique_count,
        run_unique_dense,
    ) = build_direct_dense_tables(
        vertex, runs, dense_vertices, dense_color_sources, dense_corners,
        action_dense_first, run_first_corner, run_owners,
        run_root_bindings, run_binding_sets, owner_cross_slots,
    )

    lines = [
        "/* Generated by scripts/generate_nds_native_owners.py. */",
        "/* Canonical export: 32 roots, 49 epochs, 67 runs, 626 triangles. */",
        "/* Dense geometry: 541 immutable vertices, 1878 indexed corners. */",
        "",
    ]
    lines += emit_rows(
        "NDSNativeStateDelta", "sNdsNativeFighterStateDeltas",
        [f"{{ 0x{w0:08x}u, 0x{w1:08x}u, {effect}u, {{ 0u, 0u, 0u }} }}"
         for w0, w1, effect in state],
    )
    lines += emit_rows(
        "u8", "sNdsNativeFighterStateSequence",
        [f"{value}u" for value in sequence],
    )
    lines += emit_rows(
        "NDSNativeVertexAction", "sNdsNativeFighterVertexActions",
        [f"{{ {kind}u, {command}u, {index}u, {count}u, 0x{offset:08x}u, {s}, {t} }}"
         for kind, command, index, count, offset, s, t in vertex],
    )
    lines += ["#if NDS_RENDERER_HW_TRIANGLES", ""]
    lines += [
        "/* Direct policy: low two epoch bits select a family; bit 0x80 */",
        "/* suppresses culling. Packed dense IDs occupy bits 0..9 and */",
        "/* packed GX slots occupy bits 10..14; slot 31=logical current */",
        "/* root, restored through that binding's real palette slot. */",
        "/* ActionDenseSpans pack first dense in bits 0..9 and count in */",
        "/* bits 10..14. DenseColorSource preserves MODIFY_ST shading. */",
        "/* JointSchedule packs parent joint, binding, and GX slot into */",
        "/* successive 5-bit fields; 31 means none/root/current by field. */",
        "",
    ]
    policy_flag_expressions = {
        "VERTEX|TEXTURE":
            "NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX | "
            "NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE",
        "MATERIAL|VERTEX":
            "NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL | "
            "NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX",
        "VERTEX": "NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX",
    }
    lines += emit_rows(
        "NDSNativeDirectPolicy", "sNdsNativeFighterDirectPolicies",
        ["{{ 0x{:08x}u, 0x{:08x}u, {}, {}u, {{ 0u, 0u }} }}".format(
            combine_w0, combine_w1, policy_flag_expressions[flags], textured)
         for combine_w0, combine_w1, flags, textured in
         DIRECT_POLICY_FAMILIES],
    )
    lines += emit_rows(
        "u8", "sNdsNativeFighterEpochDirectPolicy",
        [f"0x{value:02x}u" for value in direct_epoch_policies],
    )
    lines += emit_rows(
        "NDSNativeDenseVertex", "sNdsNativeFighterDenseVertices",
        [f"{{ {x}, {y}, {z}, {s}, {t}, {binding}u, {cache_slot}u, "
         f"0x{rgba:08x}u }}"
         for x, y, z, s, t, binding, cache_slot, rgba in dense_vertices],
    )
    lines += emit_rows(
        "u16", "sNdsNativeFighterActionDenseFirst",
        [f"{value}u" for value in action_dense_first],
    )
    lines += emit_rows(
        "u16", "sNdsNativeFighterActionDenseSpans",
        [f"0x{value:04x}u" for value in action_dense_spans],
    )
    lines += emit_rows(
        "u16", "sNdsNativeFighterDenseColorSource",
        [f"{value}u" for value in dense_color_sources],
    )
    lines += emit_rows(
        "u16", "sNdsNativeFighterDenseCorners",
        [f"{value}u" for value in dense_corners],
    )
    lines += emit_rows(
        "u16", "sNdsNativeFighterPackedCorners",
        [f"0x{value:04x}u" for value in packed_corners],
    )
    lines += emit_rows(
        "u16", "sNdsNativeFighterRunFirstCorner",
        [f"{value}u" for value in run_first_corner],
    )
    lines += emit_rows(
        "u16", "sNdsNativeFighterRunFirstUnique",
        [f"{value}u" for value in run_first_unique],
    )
    lines += emit_rows(
        "u8", "sNdsNativeFighterRunUniqueCount",
        [f"{value}u" for value in run_unique_count],
    )
    lines += emit_rows(
        "u16", "sNdsNativeFighterRunUniqueDense",
        [f"{value}u" for value in run_unique_dense],
    )
    for owner_index, (owner_name, roots) in enumerate(owner_roots):
        joint_schedule, binding_parents, binding_joints, cross_slots = (
            owner_topologies[owner_index]
        )
        owner_title = owner_name.title()
        lines += emit_rows(
            "u8", f"sNdsNative{owner_title}CrossPaletteSlots",
            [f"{value}u" for value in cross_slots],
        )
        lines += emit_rows(
            "u8", f"sNdsNative{owner_title}BindingParents",
            [f"{value}u" for value in binding_parents],
        )
        lines += emit_rows(
            "u8", f"sNdsNative{owner_title}BindingJoints",
            [f"{value}u" for value in binding_joints],
        )
        lines += emit_rows(
            "u16", f"sNdsNative{owner_title}JointSchedule",
            [f"0x{value:04x}u" for value in joint_schedule],
        )
    lines += ["#endif", ""]
    lines += emit_rows(
        "u16", "sNdsNativeFighterTriangles",
        [f"0x{value:04x}u" for value in triangles],
    )
    lines += emit_rows(
        "NDSNativeRun", "sNdsNativeFighterRuns",
        [f"{{ {first}u, {count}u, {submit_class}u, 0x{mask:08x}u }}"
         for first, count, submit_class, mask in runs],
    )
    lines += emit_rows(
        "NDSNativeEpoch", "sNdsNativeFighterEpochs",
        ["{{ {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u }}".format(*row)
         for row in epochs],
    )
    root_format = (
        "{{ 0x{:08x}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u }}"
    )
    lines += emit_rows(
        "NDSNativeRoot", "sNdsNativeMarioRoots",
        [root_format.format(*row) for row in mario_roots],
    )
    lines += emit_rows(
        "NDSNativeRoot", "sNdsNativeFoxRoots",
        [root_format.format(*row) for row in fox_roots],
    )
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--output", type=Path,
        default=Path(__file__).resolve().parents[1]
        / "src" / "nds" / "nds_native_fighter_owner.generated.inc",
    )
    parser.add_argument(
        "--source-root", type=Path,
        default=Path(__file__).resolve().parents[1],
        help="repo root containing the read-only BattleShip O2R inputs",
    )
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    generated = generate(args.source_root)
    if args.check:
        if not args.output.is_file() or args.output.read_text() != generated:
            raise SystemExit(f"stale generated native-owner IR: {args.output}")
        return 0
    args.output.parent.mkdir(parents=True, exist_ok=True)
    if not args.output.is_file() or args.output.read_text() != generated:
        args.output.write_text(generated)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
