#!/usr/bin/env python3
"""Qualified EXTRA geometry replacement through the existing FPC2/BEX2 ABI.

Retain Main in its entirety. Retain every Model byte except source Gfx/Vtx
spans consumed by the hash-qualified native IR, including electric variants.
Unknown bytes remain live; no guessed C declarations or pruning classes exist.
Motion/event/figatree files retain the established status/animation lifetimes.
"""
import hashlib
import json
from pathlib import Path
import struct

import estimate_fighter_pack as est
import generate_nds_fighter_admission as admission
import generate_preview_core_packs as fpc
import extra_native_asset_adapter as assets
from extra_texture_admission import MetaTextureClosure, MAIN_ID, MODEL_ID, MOTION_ID


def merge_ranges(ranges):
    result = []
    for start, end in sorted(ranges):
        if start < 0 or end <= start:
            raise fpc.PackError('Meta replacement has invalid source span')
        if result and start <= result[-1][1]:
            result[-1] = (result[-1][0], max(end, result[-1][1]))
        else:
            result.append((start, end))
    return result


def qualify_native_ir(document, closure):
    if (document.get('schema_version') != 1 or document.get('character') != 'Meta Knight' or
            document.get('status') != 'NATIVE_MODEL_IR_NOT_RUNTIME_ADMISSION'):
        raise fpc.PackError('Meta core: native IR admission identity changed')
    resource = next((row for row in document['resources'] if row['name'] == 'CHARACTER'), None)
    if resource is None or resource['sha256'] != hashlib.sha256(closure.payload(MODEL_ID)).hexdigest():
        raise fpc.PackError('Meta core: native IR and Model source bytes differ')
    entries = []
    if set(document.get('model_ir', {})) != {'high', 'low'}:
        raise fpc.PackError('Meta core: both native base details are required')
    entries.extend(document['model_ir'].values())
    if set(document.get('skeleton_ir', {})) != {'1', '2'}:
        raise fpc.PackError('Meta core: electric skeleton native IR coverage is missing')
    for variant in document['skeleton_ir'].values():
        if set(variant) != {'high', 'low'}:
            raise fpc.PackError('Meta core: electric variant lacks a native detail')
        entries.extend(variant.values())
    qualified_roots = set()
    hashes = []
    for entry in entries:
        context = entry['ir']
        encoded = json.dumps(context, sort_keys=True, separators=(',', ':'), allow_nan=False).encode()
        if hashlib.sha256(encoded).hexdigest() != entry['ir_sha256']:
            raise fpc.PackError('Meta core: native IR hash mismatch')
        if entry['source_asset_id'] != MODEL_ID or context['source_asset_id'] != MODEL_ID:
            raise fpc.PackError('Meta core: native IR has a foreign Model identity')
        if len(context['roots']) != len(entry['source_roots']):
            raise fpc.PackError('Meta core: native root census differs from source roots')
        qualified_roots.update(row['offset'] for row in entry['source_roots'])
        hashes.append(entry['ir_sha256'])
    required = {row[0][1] for detail in (0, 1) for row in closure.native_part_rows(detail)}
    if required != qualified_roots:
        raise fpc.PackError('Meta core: native IR does not cover the complete typed root union')
    return qualified_roots, hashes


def build_meta_pack(native_dir, model_ir=None, *, with_menu_sections=False):
    native_dir = Path(native_dir)
    closure = MetaTextureClosure(native_dir, admission)
    if model_ir is None:
        document = json.loads((native_dir / 'meta-knight-model-ir.json').read_text())
    elif isinstance(model_ir, dict):
        document = model_ir
    else:
        document = json.loads(Path(model_ir).read_text())
    roots, ir_hashes = qualify_native_ir(document, closure)
    model = closure.resources[MODEL_ID]
    main = closure.resources[MAIN_ID]
    gfx = [(obj.offset, obj.offset + obj.size) for obj in closure.objects[MODEL_ID]
           if obj.type_name == 'Gfx']
    vertices = []
    for start, end in gfx:
        for slot in range(start + 4, end, 8):
            word = closure.u32(MODEL_ID, slot - 4)
            if word >> 24 != 0x01:
                continue
            pointer = model.internal.get(slot)
            if pointer is None:
                raise fpc.PackError('Meta native vertex span has no local source identity')
            count = word >> 12 & 0xFF
            assets.span(model.payload, pointer.offset, count * 16, 'Meta native Vtx')
            vertices.append((pointer.offset, pointer.offset + count * 16))
    removed = merge_ranges(gfx + vertices)
    textures = admission.enumerate_kind('MetaKnight', MAIN_ID, closure)
    closure.validate_records(textures)
    protected = [(file_id, offset, size) for file_id, offset, size, _role in closure.texture_spans]
    protected += [(*ref, 120) for ref in closure.material_refs]
    protected += closure.material_stream_spans()
    for detail in closure.model['details'].values():
        protected.append((MODEL_ID, detail['tree_offset'], len(detail['descriptors']) * 44))
    for file_id, offset, size in protected:
        if file_id == MODEL_ID and any(offset < end and start < offset + size for start, end in removed):
            raise fpc.PackError(f'Meta replaced geometry overlaps required material/texture/descriptor atom {offset:#x}+{size:#x}')
    kept = []
    cursor = 0
    for start, end in removed:
        if start > cursor:
            kept.append((cursor, start))
        cursor = end
    if cursor < len(model.payload):
        kept.append((cursor, len(model.payload)))
    if any(start % 4 or end % 4 for start, end in kept):
        raise fpc.PackError('Meta structural complement lost u32 alignment')
    model_body = b''.join(model.payload[start:end] for start, end in kept)
    main_size = len(main.payload)
    pointer_map = []
    relative = 0
    for start, end in kept:
        pointer_map.append({'old': start, 'len': end - start, 'new': main_size + relative})
        relative += end - start

    def retained(offset):
        return any(start <= offset < end for start, end in kept)

    def mapped(offset):
        for span in pointer_map:
            if span['old'] <= offset < span['old'] + span['len']:
                return span['new'] + offset - span['old']
        raise fpc.PackError(f'Meta source structural target {offset:#x} is outside retained spans')

    def model_edge(slot, target):
        edge = {'slot_old': slot, 'slot_new': mapped(slot), 'target_old': target}
        if retained(target):
            edge['target_new'] = mapped(target)
        elif target in roots:
            edge.update(target_new='sentinel', reason='pruned-dl')
        else:
            raise fpc.PackError(f'Meta retained pointer {slot:#x} targets unclassified replaced atom {target:#x}')
        return edge

    model_internal = [model_edge(slot, pointer.offset) for slot, pointer in model.internal.items()
                      if retained(slot)]
    if model.external:
        raise fpc.PackError('Meta Model external closure requires explicit section mapping')
    main_internal = [{'slot_old': slot, 'slot_new': slot, 'target_old': pointer.offset,
                      'target_new': pointer.offset, 'target_file': MAIN_ID}
                     for slot, pointer in main.internal.items()]
    main_kept, main_pruned, patches = [], [], []
    manifest_assets = {row['native_file_id']: row for row in closure.bindings['assets']}
    required_external = set()
    for slot, pointer in main.external.items():
        dep, target = pointer.dependency.file_id, pointer.offset
        edge = {'slot_old': slot, 'slot_new': slot, 'target_old': target, 'target_file': dep}
        if dep == MODEL_ID:
            if retained(target):
                edge['target_new'] = mapped(target)
                main_kept.append(edge)
            elif target in roots:
                edge['reason'] = 'pruned-dl'
                main_pruned.append(edge)
            else:
                raise fpc.PackError(f'Meta Main model target {target:#x} lacks qualified source semantics')
        else:
            if dep not in manifest_assets or target >= manifest_assets[dep]['size']:
                raise fpc.PackError('Meta Main external pointer has no qualified staged dependency')
            blob = (native_dir / manifest_assets[dep]['path']).read_bytes()
            if hashlib.sha256(blob).hexdigest() != manifest_assets[dep]['container_sha256']:
                raise fpc.PackError('Meta Main dependency changed after native qualification')
            edge['reason'] = 'external-source-resolved-via-bex'
            main_pruned.append(edge)
            patches.append((slot, dep, target))
            required_external.add(dep)
    metadata = {
        'main_intern': {'retained': main_internal},
        'main_extern': {'kept': main_kept, 'pruned': main_pruned},
        'model_intern': {'retained': model_internal, 'reloc_file': '5456_MetaKnightModel'},
        'model_extern': {}, 'tail_files': [], 'pointer_map': pointer_map,
        'section_boundaries': {'main': [0, main_size]},
        'sections': [{'src': 'Main-image', 'old': 0, 'new': 0, 'len': main_size, 'mode': 'identity'}],
        'checks': {'main_sections_sum': main_size, 'model_payload_bytes': len(model.payload),
                   'model_source_bytes': len(model.payload)},
        'pruned_dl_roots': [{'target_old': offset} for offset in sorted(roots)],
    }
    raw_compact = main.payload + model_body
    motion = closure.resources[MOTION_ID]
    menu_ids = [closure.u32(MOTION_ID, (225 + index) * 12) for index in (0, 4)]
    for file_id in menu_ids:
        if file_id not in manifest_assets or not manifest_assets[file_id]['animation']:
            raise fpc.PackError('Meta source CSS idle/selected menu clip is not staged and admitted')
    if menu_ids[1] != 5551:
        raise fpc.PackError('Meta source selected Win4 clip identity changed')
    if with_menu_sections:
        menu_payloads = []
        for label, file_id in zip(('anim-idle', 'anim-selected'), menu_ids):
            _, animation = assets.load_o2r(native_dir / manifest_assets[file_id]['path'])
            metadata['sections'].append({'name': label, 'new': len(raw_compact), 'len': len(animation.payload)})
            raw_compact += animation.payload
            menu_payloads.append(animation.payload)
        metadata['idle'] = {'file': menu_ids[0], 'bytes': len(menu_payloads[0])}
        metadata['selected'] = {'file': menu_ids[1]}
        metadata['selected_proof'] = {'total_bytes': len(menu_payloads[1])}
    blob, report = fpc.build_pack('metaknight', 29, metadata, raw_compact,
                                  with_menu_sections=with_menu_sections)
    decoded = fpc.decode_pack(blob)
    resident = (len(decoded['data']) + len(decoded['sections']) * 32 + len(decoded['spans']) * 12)
    census = est.parse_native_image_census(est.NATIVE_IMAGE_FLAGS_BY_NAME['hwtri'])
    image = census.get('Metaknight')
    if image is None or set(image) != {'High', 'Low'}:
        raise fpc.PackError('Meta core: qualified native image byte census is absent')
    css_peak = resident + image['High']
    if css_peak > 80 * 1024:
        raise fpc.PackError(f'Meta preview pack/native image peak {css_peak} exceeds the measured 80 KiB slot')
    # Existing BEX2 ABI; unlike legacy shield projection, Meta explicitly keeps
    # all nine original shield references and restores them through the loader.
    import generate_battle_core_packs as battle
    ext = struct.pack(battle.EXTERN_HEADER_FMT, battle.EXTERN_MAGIC, battle.EXTERN_VERSION,
                      len(patches), 0, 0, fpc.fnv1a32(b''), 0)
    ext += b''.join(struct.pack(battle.EXTERN_ROW_FMT, *row) for row in patches)
    dependencies = {}
    def visit_dependency(file_id):
        if file_id in dependencies:
            return
        entry = manifest_assets[file_id]
        _, resource = assets.load_o2r(native_dir / entry['path'])
        dependencies[file_id] = (len(resource.payload) + 15) & ~15
        for pointer in resource.external.values():
            visit_dependency(pointer.dependency.file_id)
    for file_id in required_external:
        visit_dependency(file_id)
    motion_resident = (len(motion.payload) + 15) & ~15
    report.update({'status': 'IMPLEMENTED_NOT_ACCEPTED', 'fighter': 'MetaKnight',
                   'main_asset': MAIN_ID, 'model_asset': MODEL_ID,
                   'source_rom_sha256': closure.bindings['source_rom_sha256'],
                   'resident_allocation': resident, 'native_image_bytes': image,
                   'css_pack_and_high_image_peak': css_peak, 'css_slot_limit': 80 * 1024,
                   'core_and_both_detail_images': resident + sum(image.values()),
                   'model_structural_bytes': len(model_body), 'model_replaced_geometry_bytes': sum(e-s for s,e in removed),
                   'geometry_replacement_ranges': removed, 'retained_unknown_bytes_policy': 'retain exact source complement',
                   'native_ir_hashes': ir_hashes, 'external_patches': patches,
                   'external_status_allocations': dependencies, 'motion_status_allocation': motion_resident,
                   'css_figatree_bytes': closure.bindings['max_menu_animation_size'],
                   'battle_figatree_bytes': closure.bindings['max_main_animation_size'],
                   'css_scene_peak_separate_lifetimes': css_peak + motion_resident + sum(dependencies.values()) + closure.bindings['max_menu_animation_size'],
                   'required_menu_clips': [{'menu_row': row, 'native_file_id': file_id, 'bytes': manifest_assets[file_id]['size']}
                                           for row, file_id in zip((0,4), menu_ids)],
                   'lifetime_contract': {'FPC': 'Main/Model and optional menu source bytes in existing CSS resident slot',
                                         'native_images': 'currently selected detail through existing native image cache',
                                         'motion_and_externs': 'existing status loader/cache; BEX2 must apply before construction',
                                         'figatree': 'existing source clip force loader in independently allocated figatree heap'},
                   'checks_owed': ['preview and battle BEX2 dependency publication before fighter construction',
                                   'natural CSS/selected Win4 input and playable match verification']})
    return blob, ext, report
