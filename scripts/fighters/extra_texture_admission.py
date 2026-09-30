#!/usr/bin/env python3
"""Typed EXTRA source provider for the established ADM1 texture producer.

All resource IDs are qualified by the frozen native binding manifest. Part
tables use the raw adapter's validated structures; material animation and RDP
state are evaluated by the existing fighter admission machinery.
"""
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import struct
from types import SimpleNamespace

import extra_native_asset_adapter as assets
import extra_resource_adapter as raw

MAIN_ID, MODEL_ID, MOTION_ID = assets.MAIN_ID, assets.MODEL_ID, assets.MOTION_ID
GFX_OPS = {0x01, 0x02, 0x05, 0x06, 0x07, 0xD7, 0xD9, 0xDB, 0xDE, 0xDF,
           0xE2, 0xE3, 0xE6, 0xE7, 0xE8, 0xEF, 0xF0, 0xF2, 0xF3, 0xF4,
           0xF5, 0xFA, 0xFB, 0xFC, 0xFD}


class MetaTextureClosure:
    name = 'MetaKnight'
    costume_count = 6

    def __init__(self, native_dir, admission):
        self.admission = admission
        native_dir = Path(native_dir)
        bindings = json.loads((native_dir / 'native-runtime-bindings.json').read_text())
        if bindings['schema'] != 'smash64ds.p4-native-runtime-bindings.v1':
            raise raw.AdapterError('Meta texture provider: native binding schema changed')
        self.bindings = bindings
        self.resources = {}
        for entry in bindings['assets']:
            if entry['native_file_id'] not in (MAIN_ID, MODEL_ID, MOTION_ID):
                continue
            path = native_dir / entry['path']
            if hashlib.sha256(path.read_bytes()).hexdigest() != entry['container_sha256']:
                raise raw.AdapterError(f'Meta texture provider: frozen {path.name} changed')
            file_id, resource = assets.load_o2r(path)
            if file_id != entry['native_file_id']:
                raise raw.AdapterError('Meta texture provider: source file identity changed')
            self.resources[file_id] = resource
        if set(self.resources) != {MAIN_ID, MODEL_ID, MOTION_ID}:
            raise raw.AdapterError('Meta texture provider: missing native Main/Model/Motion')
        # model_inventory operates on explicit resource names. This is a typed
        # bridge for the two already-verified native IDs, never an ID guess.
        named = {}
        for file_id, name in ((MAIN_ID, 'MAIN'), (MODEL_ID, 'CHARACTER')):
            resource = self.resources[file_id]
            deps = [raw.Dependency('symbol', name='CHARACTER')
                    if pointer.dependency.file_id == MODEL_ID else
                    raw.Dependency('symbol', name='MAIN')
                    if pointer.dependency.file_id == MAIN_ID else pointer.dependency
                    for pointer in resource.external.values()]
            named[name] = raw.decode_resource(name, resource.payload,
                                              resource.internal_head, resource.external_head, deps)
        self.model = raw.model_inventory(named['MAIN'], named['CHARACTER'], bindings['attribute_offset'])
        self.costume_count = len(self.model['stock']['stock_luts']['palettes'])
        if self.costume_count != 6:
            raise raw.AdapterError('Meta source costume/LUT domain changed from the six-costume donor')
        self.files = {file_id: SimpleNamespace(file_id=file_id, source={
            'payload': resource.payload, 'pointers': {
                slot: (pointer.dependency.file_id if pointer.dependency else file_id, pointer.offset)
                for slot, pointer in {**resource.internal, **resource.external}.items()}})
                      for file_id, resource in self.resources.items()}
        self.objects = defaultdict(list)
        self._rows = {0: [], 1: []}
        self.material_refs = set()
        self.stream_refs = set()
        self._compile_parts()
        self.targets = self._compile_texture_parts()
        self.extents = admission.mobj_table_extents(self, MAIN_ID)

    def _ref(self, pointer):
        if pointer is None:
            return None
        if 'resource' in pointer:
            name = pointer['resource']
        elif pointer.get('dependency', {}).get('kind') == 'symbol':
            name = pointer['dependency']['name']
        else:
            dep = pointer.get('dependency', {})
            file_id = dep.get('file_id')
            if file_id not in self.resources:
                raise raw.AdapterError(f'Meta texture provider: unclassified dependency {dep}')
            return file_id, pointer['offset']
        if name not in ('MAIN', 'CHARACTER'):
            raise raw.AdapterError(f'Meta texture provider: unknown typed resource {name}')
        return (MAIN_ID if name == 'MAIN' else MODEL_ID), pointer['offset']

    def payload(self, file_id):
        if file_id not in self.resources:
            raise raw.AdapterError(f'Meta texture provider: missing reached resource {file_id}')
        return self.resources[file_id].payload

    def ptr(self, file_id, slot):
        resource = self.resources[file_id]
        if slot in resource.internal:
            return file_id, resource.internal[slot].offset
        if slot in resource.external:
            pointer = resource.external[slot]
            return pointer.dependency.file_id, pointer.offset
        word = self.u32(file_id, slot)
        if word == 0:
            return None
        obj = self.obj_at(file_id, slot - 4)
        if obj is not None and obj.type_name == 'Gfx' and word >> 24 == 0x0E and word & 7 == 0:
            return None  # Explicit validated material-segment branch.
        raise raw.AdapterError(f'Meta texture provider: nonzero untyped pointer {file_id}:{slot:#x}')

    def u16(self, file_id, offset):
        raw._span(self.payload(file_id), offset, 2, 'Meta u16', 2)
        return struct.unpack_from('>H', self.payload(file_id), offset)[0]

    def u32(self, file_id, offset):
        raw._span(self.payload(file_id), offset, 4, 'Meta u32')
        return struct.unpack_from('>I', self.payload(file_id), offset)[0]

    def obj_at(self, file_id, offset):
        matches = [obj for obj in self.objects.get(file_id, ())
                   if obj.offset <= offset < obj.offset + obj.size]
        if not matches:
            return None
        return min(matches, key=lambda obj: obj.size)

    def _object(self, file_id, offset, size, type_name):
        raw._span(self.payload(file_id), offset, size, 'Meta typed ' + type_name)
        for obj in self.objects[file_id]:
            if obj.offset == offset and obj.type_name == type_name:
                if obj.size != size:
                    raise raw.AdapterError('Meta typed object has conflicting source extents')
                return
        self.objects[file_id].append(SimpleNamespace(offset=offset, size=size,
                                                     type_name=type_name, symbol=None))

    def raw_at(self, ref, index):
        return self.ptr(ref[0], ref[1] + 4 * index) if ref else None

    def mobjsubs(self, ref):
        result = []
        for index in range(64):
            pointer = self.raw_at(ref, index)
            if pointer is None:
                return result
            self._object(*pointer, raw.MOBJSUB_SIZE, 'MObjSub')
            self.material_refs.add(pointer)
            result.append(pointer)
        raise raw.AdapterError('Meta MObjSub pointer chain has no source null terminator')

    def _gfx(self, ref):
        known = self.obj_at(*ref)
        if known is not None:
            if known.type_name != 'Gfx':
                raise raw.AdapterError('Meta Gfx pointer crosses a different typed source object')
            return
        file_id, offset = ref
        resource = self.resources[file_id]
        info = raw._display_root(resource, offset)
        unknown = {op['opcode'] for op in info['opcodes']} - GFX_OPS
        if unknown:
            raise raw.AdapterError(f'Meta Gfx: unclassified semantic atoms {sorted(unknown)}')
        for position in range(offset, offset + info['command_count'] * 8, 8):
            word = self.u32(file_id, position)
            if word >> 24 == 0x02 and (((word >> 16) & 0xFF) != 0x14 or
                                      word & 1 or (word & 0xFFFF) // 2 >= 32):
                raise raw.AdapterError('Meta electric ModifyVtx has unknown source field/index semantics')
        self._object(file_id, offset, info['command_count'] * 8, 'Gfx')
        for branch in info['branches']:
            if 'material_slot' in branch:
                continue
            pointer = self.ptr(file_id, branch['slot'])
            if pointer is None:
                raise raw.AdapterError('Meta Gfx: missing typed branch target')
            self._gfx(pointer)

    def _add(self, detail, root, materials, costume, main, where):
        if root is None:
            return
        self._gfx(root)
        mobjs = self.mobjsubs(materials) if materials else []
        cl = [self.raw_at(costume, index) for index in range(len(mobjs))] if costume else []
        ml = [self.raw_at(main, index) for index in range(len(mobjs))] if main else []
        self.stream_refs.update(pointer for pointer in cl + ml if pointer)
        row = (root, mobjs, cl, ml, where)
        if row not in self._rows[detail]:
            self._rows[detail].append(row)

    def _compile_parts(self):
        details = self.model['details']
        for detail, name in enumerate(('high', 'low')):
            part = details[name]
            roots = part['canonical_roots'] + part['hidden_roots']
            for root in roots:
                index = root['descriptor_index']
                chosen = name if name == 'high' or part['descriptors'][index]['display_list'] else 'high'
                material_dispatch = self._ref(details[chosen]['material_dispatch'])
                costume_dispatch = self._ref(details[chosen]['costume_dispatch'])
                materials = self.raw_at(material_dispatch, index)
                costume = self.raw_at(costume_dispatch, index)
                self._add(detail, self._ref(root), materials, costume, None, ('joint', index))
        for record in self.model['modelparts']['records']:
            if record['flags'] & 0xF:
                raise raw.AdapterError('Meta modelpart: unsupported paired/custom display semantics')
            self._add(0 if record['detail'] == 'high' else 1,
                      self._ref(record['display_list']), self._ref(record['mobjsubs']),
                      self._ref(record['costume_matanim_joints']),
                      self._ref(record['main_matanim_joints']),
                      ('mp', record['joint_id'] - 4, record['modelpart_id'],
                       (MAIN_ID, self.model['modelparts']['offset'])))
        access = self.model['accesspart']
        if access is not None:
            for detail in (0, 1):
                self._add(detail, self._ref(access['display_list']), self._ref(access['mobjsubs']),
                          self._ref(access['costume_matanim_joints']), None,
                          ('access', access['joint_id'] - 4))
        # Recorded coverage invalidator: both source electric skeleton
        # selectors replace seven common-joint lists and can carry additional
        # image/material state. They inherit that joint's live MObj chain.
        skeletons = self.model.get('skeletons', {}).get('variants', {})
        if set(skeletons) != {'1', '2'}:
            raise raw.AdapterError('Meta electric skeleton source variants are incomplete')
        self.inherited_skeleton_materials = []
        for skeleton_id, variant in skeletons.items():
            for detail, name in enumerate(('high', 'low')):
                # ftDisplayMainDrawSkeleton traverses the source tree in
                # descriptor order. gcDrawMObjForDObj leaves segment E intact
                # when this DObj's chain is NULL. A visibility change can skip
                # a preceding binder, so admit every preceding source binder;
                # the renderer still selects the actual material DObj at draw.
                preceding_binders = []
                for root in variant['roots']:
                    index = root['descriptor_index']
                    part = details[name]
                    chosen = name if name == 'high' or part['descriptors'][index]['display_list'] else 'high'
                    materials = self.raw_at(self._ref(details[chosen]['material_dispatch']), index)
                    costume = self.raw_at(self._ref(details[chosen]['costume_dispatch']), index)
                    chain = self.mobjsubs(materials) if materials else []
                    if chain:
                        preceding_binders.append((index, materials, costume))
                        choices = [(index, materials, costume)]
                    elif any('material_slot' in branch for branch in root['branches']):
                        choices = preceding_binders
                        if not choices:
                            raise raw.AdapterError('Meta skeleton material call has no qualified preceding binder')
                        self.inherited_skeleton_materials.append({
                            'skeleton_id': int(skeleton_id), 'detail': name, 'root': root['offset'],
                            'joint_id': root['joint_id'],
                            'source_binder_joints': [binder[0] + 4 for binder in choices]})
                    else:
                        choices = [(index, materials, costume)]
                    for binder_index, binder_materials, binder_costume in choices:
                        self._add(detail, self._ref(root), binder_materials, binder_costume,
                                  None, ('joint', binder_index))

    def native_part_rows(self, detail):
        return self._rows[detail]

    def _compile_texture_parts(self):
        motion = self.resources[MOTION_ID]
        compiler = assets.EventCompiler(SimpleNamespace(load=lambda file_id: self.resources[file_id]),
                                         b'', {}, mainmotion_id=MOTION_ID)
        total = self.bindings['mainmotion_count'] + self.bindings['menu_count']
        for index in range(total):
            offset = self.u32(MOTION_ID, index * 12 + 4)
            if offset != assets.NONE:
                compiler.visit(assets.Address('file', MOTION_ID, offset))
        ids = defaultdict(set)
        for address, payload in compiler.nodes.items():
            if compiler.classification[address] != 'script':
                continue
            word = struct.unpack_from('>I', payload)[0]
            if word >> 26 == 43:
                part, texture_id = (word >> 20) & 0x3F, word & 0xFFFFF
                if part >= 2 or texture_id >= 256:
                    raise raw.AdapterError('Meta SetTexturePartID exceeds source texture-part domain')
                ids[part].add(texture_id)
        target = self._ref(self.model['pointer_fields']['textureparts'])
        targets = defaultdict(lambda: defaultdict(set))
        if target is None and ids:
            raise raw.AdapterError('Meta SetTexturePartID has no source container')
        data = self.payload(target[0]) if target else b''
        for part, values in ids.items():
            at = target[1] + part * 3
            raw._span(data, at, 3, 'Meta FTTexturePart', 1)
            joint = data[at] - 4
            if not 0 <= joint < len(self.model['details']['high']['descriptors']) - 1:
                raise raw.AdapterError('Meta texture part names absent source joint')
            for detail in (0, 1):
                targets[joint, detail][data[at + 1 + detail]].update(values | {0})
        self.texture_part_ids = {part: sorted(values) for part, values in ids.items()}
        return targets

    def native_texture_part_targets(self):
        return self.targets

    def material_stream_spans(self):
        """Typed words consumed by every reached AObj32 material program."""
        seen, spans = set(), []
        def visit(ref):
            while ref not in seen:
                seen.add(ref)
                word = self.u32(*ref)
                op, flags = word >> 25, (word >> 15) & 0x3FF
                if op in (0, 2, 12):
                    words = 1
                elif op in (1, 14):
                    words = 2
                elif op in (3, 4, 8, 9, 10, 11, 7, 18, 19, 20, 21):
                    words = 1 + flags.bit_count()
                elif op in (5, 6):
                    words = 1 + 2 * flags.bit_count()
                elif op == 22:
                    words = 1 + (flags & 31).bit_count()
                else:
                    raise raw.AdapterError(f'Meta material stream contains unknown opcode {op}')
                raw._span(self.payload(ref[0]), ref[1], words * 4, 'Meta material stream words')
                spans.append((ref[0], ref[1], words * 4))
                if op == 0:
                    return
                if op in (1, 14):
                    target = self.ptr(ref[0], ref[1] + 4)
                    if target is None:
                        raise raw.AdapterError('Meta material stream branch is null')
                    ref = target
                else:
                    ref = ref[0], ref[1] + words * 4
        for ref in sorted(self.stream_refs):
            visit(ref)
        return spans

    def validate_records(self, tables):
        self.texture_spans = set()
        for records in tables.values():
            for record in records.values():
                fmt = record['render_set'][0] >> 21 & 7
                siz = record['render_set'][0] >> 19 & 3
                if (fmt, siz) not in ((0, 2), (0, 3), (2, 0), (2, 1), (3, 0), (3, 1), (3, 2), (4, 0), (4, 1)):
                    raise raw.AdapterError(f'Meta texture: unsupported source format {fmt}/{siz}')
                if record['costume'] & ~0x3F:
                    raise raw.AdapterError('Meta texture record reaches nonexistent costume')
                if record['combine'] == (0xFC321803, 0xFF17FFFF) and (fmt, siz) != (3, 1):
                    raise raw.AdapterError('Meta alpha-only combiner requires source IA8')
                # The RDP load span, not the padded DS upload area, bounds the
                # source read. LOADBLOCK's count is in source texels at size16.
                lw0, lw1 = record['load_w']
                pixels = ((lw1 >> 12) & 0xFFF) + 1 if record['load_op'] == 0xF3 else (
                    ((((lw1 >> 12) & 0xFFF) - ((lw0 >> 12) & 0xFFF)) >> 2) + 1) * (
                    (((lw1 & 0xFFF) - (lw0 & 0xFFF)) >> 2) + 1)
                # Fighter LoadBlock's count is in 16-bit transfer units even
                # for CI4/IA8. Material branches may leave tile7 implicit, so
                # a missing SETTILE cannot be mistaken for a 4-bit load. This
                # is the same loaded-byte convention ds_dims qualifies.
                byte_count = pixels * (4 if siz == 3 else 2)
                image = record['img']
                raw._span(self.payload(image[0]), image[1], byte_count, 'Meta texture load', 1)
                self.texture_spans.add((*image, byte_count, 'image'))
                if fmt == 2 and record['tlut'] is None:
                    raise raw.AdapterError('Meta indexed texture has no source palette')
                if record['tlut']:
                    palette = record['tlut']
                    count = ((record['loadtlut_w1'] >> 14) & 0x3FF) + 1
                    raw._span(self.payload(palette[0]), palette[1], count * 2, 'Meta TLUT', 2)
                    self.texture_spans.add((*palette, count * 2, 'palette'))

    def report(self, tables):
        by_resource = defaultdict(list)
        for file_id, offset, size, _role in self.texture_spans:
            by_resource[file_id].append((offset, offset + size))
        union = []
        for file_id, spans in sorted(by_resource.items()):
            merged = []
            for start, end in sorted(spans):
                if merged and start <= merged[-1][1]:
                    merged[-1] = (merged[-1][0], max(end, merged[-1][1]))
                else:
                    merged.append((start, end))
            union.extend({'resource': file_id, 'offset': start, 'bytes': end - start}
                         for start, end in merged)
        alpha_only = []
        for detail, name in enumerate(('high', 'low')):
            for record in tables[detail].values():
                if record['combine'] != (0xFC321803, 0xFF17FFFF):
                    continue
                file_id, offset = record['img']
                size = self.admission._source_bytes(3, 1, record['dims'][0] * record['dims'][1])
                alpha_values = sorted({value & 15 for value in self.payload(file_id)[offset:offset + size]})
                alpha_only.append({'detail': name, 'resource': file_id, 'offset': offset,
                                   'bytes': size, 'source_format': 'IA8', 'view_key_bit': 28,
                                   'representation': 'A5/I3 with white RGB',
                                   'source_alpha_nibbles': alpha_values})
        return {'schema': 'smash64ds.p4-meta-texture-admission.v1',
                'status': 'IMPLEMENTED_NOT_ACCEPTED', 'runtime_kind': 29, 'admission_index': 12,
                'source_rom_sha256': self.bindings['source_rom_sha256'], 'costume_count': 6,
                'record_words': self.admission.RECORD_WORDS,
                'details': {name: {'records': len(tables[detail]),
                                  'record_bytes': len(tables[detail]) * self.admission.RECORD_WORDS * 4,
                                  'part_rows': len(self._rows[detail]),
                                  'roots': len({row[0] for row in self._rows[detail]})}
                            for detail, name in enumerate(('high', 'low'))},
                'material_count': len(self.material_refs), 'material_stream_count': len(self.stream_refs),
                'costume_provenance': 'six source FTSprites stock LUT pointers; same admitted costume domain',
                'alpha_only_views': alpha_only,
                'inherited_skeleton_materials': self.inherited_skeleton_materials,
                'material_table_extents': [{'resource': ref[0], 'offset': ref[1],
                                            'texture_id_max': limits[0], 'palette_id_max': limits[1]}
                                           for ref, limits in sorted(self.extents.items())],
                'texture_part_ids': self.texture_part_ids,
                'source_texture_spans': [{'resource': file_id, 'offset': offset, 'bytes': size, 'role': role}
                                         for file_id, offset, size, role in sorted(self.texture_spans)],
                'resource_union': sorted({span[0] for span in self.texture_spans}),
                'source_texture_bytes_sum': sum(span[2] for span in self.texture_spans),
                'source_texture_union_bytes': sum(span['bytes'] for span in union),
                'source_texture_union_spans': union,
                'checks_owed': ['pre-GO admission through existing texture cache',
                                'all six costumes and playable-match visual verification']}
