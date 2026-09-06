"""Run the production FGM loader against the real pack and corrupt inputs."""
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/menus'))
from source_test_helpers import braced, function


class FgmMetadataTest(unittest.TestCase):
    def test_resident_loader_and_failures(self):
        source = (ROOT/'src/nds/nds_audio_fgm.c').read_text()
        header = (ROOT/'include/nds/nds_audio_fgm.h').read_text()
        pack = (ROOT/'assets/audio/fgm_phase_pack_ima.bin').read_bytes()
        entry = braced(source, r'typedef struct NDSAudioFgmPackEntry\s*\{', True)
        a = source.index('_Static_assert(__BYTE_ORDER__')
        layout = source[a:source.index(';', a)+1]
        functions = '\n'.join(function(source, name) for name in (
            'ndsAudioFgmReadLe16', 'ndsAudioFgmReadLe32',
            'ndsAudioFgmValidateCachedEntry', 'ndsAudioFgmLoadFenced'))
        defines = '\n'.join(re.search(r'^#define '+name+r'\s+[^\n]+',
                            header, re.M)[0] for name in (
            'NDS_AUDIO_FGM_ENTRY_COUNT', 'NDS_AUDIO_FGM_PACK_BYTES',
            'NDS_AUDIO_FGM_PACK_MAPPING_SHA256_LO', 'NDS_AUDIO_FGM_CACHE_BYTES',
            'NDS_AUDIO_FGM_HANDLE_CAPACITY', 'NDS_AUDIO_FGM_PASS'))
        prelude = r'''
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
typedef int32_t s32;
#define TRUE 1
#define FALSE 0
#define NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS 0
#define NDS_AUDIO_FGM_PACK_HEADER_BYTES 16u
#define NDS_AUDIO_FGM_PACK_ENTRY_BYTES 32u
#define NDS_AUDIO_FGM_PACK_DATA_OFFSET (16u+32u*NDS_AUDIO_FGM_ENTRY_COUNT)
#define NDS_AUDIO_FGM_HANDLE_COUNT NDS_AUDIO_FGM_HANDLE_CAPACITY
#define NDS_AUDIO_FGM_CACHE_MAX_ENVELOPE_POINTS 32u
#define NDS_AUDIO_FGM_ENVELOPE_POINT_BYTES 4u
#define NDS_AUDIO_FGM_FLAG_LOOP 1u
#define NDS_AUDIO_FGM_FLAG_PAUSE_WITH_GAME 2u
#define NDS_AUDIO_FGM_MASK_PACK_LOADED 1u
#define NDS_AUDIO_FGM_EXPECTED_FIDELITY_DEBT_MASK 0u
static const char *pack_path;
#define NDS_AUDIO_FGM_PATH pack_path
'''
        declarations = '\n'.join('static u32 '+name+';' for name in sorted(
            set(re.findall(r'\bgNds\w+', functions))))
        program = prelude + defines + '\n' + entry + '\n' + layout + '\n' + declarations + r'''
static NDSAudioFgmPackEntry sNdsAudioFgmEntries[NDS_AUDIO_FGM_ENTRY_COUNT];
static struct { int channel, cache_slot; } sNdsAudioFgmHandles[NDS_AUDIO_FGM_HANDLE_COUNT];
static FILE *sNdsAudioFgmFile;
static unsigned cache_resets, read_calls, short_read;
static void ndsAudioFgmCacheReset(void) { ++cache_resets; }
static size_t injected_read(void *out, size_t size, size_t count, FILE *f) {
    ++read_calls;
    if (short_read && read_calls == short_read) return fread(out, size, count-1, f);
    return fread(out, size, count, f);
}
#define fread injected_read
''' + functions + r'''
int main(int argc, char **argv) {
    if (argc < 2) return 2;
    pack_path = argv[1]; short_read = argc > 3 ? (unsigned)atoi(argv[3]) : 0;
    ndsAudioFgmLoadFenced();
    if (!gNdsAudioFgmLoaded) {
        if (cache_resets || sNdsAudioFgmFile || gNdsAudioFgmSupportedCount) return 3;
        if (gNdsAudioFgmReadFailCount) {
            for (size_t i=0; i<sizeof(sNdsAudioFgmEntries); ++i)
                if (((u8 *)sNdsAudioFgmEntries)[i]) return 4;
            puts("READ");
        } else if (gNdsAudioFgmFormatFailCount) puts("FORMAT");
        else puts("OPEN");
        return 0;
    }
    ndsAudioFgmLoadFenced(); /* Reload fence must preserve resident handles. */
    if (cache_resets != 1 || read_calls != 2 ||
        gNdsAudioFgmSupportedCount != NDS_AUDIO_FGM_ENTRY_COUNT ||
        gNdsAudioFgmResidentBytes != NDS_AUDIO_FGM_CACHE_BYTES + sizeof(sNdsAudioFgmEntries) ||
        gNdsAudioFgmHandleCapacity != NDS_AUDIO_FGM_HANDLE_COUNT) return 5;
    for (u32 i=0; i<NDS_AUDIO_FGM_HANDLE_COUNT; ++i)
        if (sNdsAudioFgmHandles[i].channel != -1 || sNdsAudioFgmHandles[i].cache_slot != -1) return 6;
    fclose(sNdsAudioFgmFile);
    /* Use a file to preserve binary bytes on the Windows host. */
    FILE *out=fopen(argv[2], "wb");
    if (!out) return 7;
    fwrite(sNdsAudioFgmEntries, 1, sizeof(sNdsAudioFgmEntries), out); fclose(out);
    puts("PASS"); return 0;
}
'''
        cc = shutil.which('gcc') or shutil.which('clang')
        self.assertIsNotNone(cc)
        with tempfile.TemporaryDirectory() as directory:
            d = Path(directory)
            cfile, exe, asset, output = [d/n for n in ('loader.c','loader.exe','pack.bin','entries.bin')]
            cfile.write_text(program)
            result = subprocess.run([cc,'-std=c11','-Wall','-Wextra','-Werror',str(cfile),'-o',str(exe)], capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr.decode())
            def run(data, expected, fault=0):
                asset.write_bytes(data)
                cmd=[str(exe),str(asset),str(output)]+([str(fault)] if fault else [])
                result=subprocess.run(cmd,capture_output=True)
                self.assertEqual(result.returncode,0,result.stderr.decode())
                self.assertEqual(result.stdout.strip(),expected.encode())
            run(pack, 'PASS')
            count=struct.unpack_from('<H',pack,6)[0]
            self.assertEqual(output.read_bytes(),pack[16:16+count*32])
            run(pack,'READ',1)
            run(pack,'READ',2)
            run(pack[:-1],'FORMAT')
            for offset, fmt, value in (
                (0,'I',0), (4,'H',0), (6,'H',0), (8,'I',0), (12,'I',0),
                (18,'H',0x8000), (20,'I',0), (24,'I',3), (28,'I',0),
                (32,'H',0), (34,'H',0), (36,'B',128), (37,'B',0),
                (44,'H',33), (46,'H',0xffff),
                (48,'H',struct.unpack_from('<H',pack,16)[0]),
            ):
                with self.subTest(offset=offset):
                    bad=bytearray(pack); struct.pack_into('<'+fmt,bad,offset,value)
                    run(bad,'FORMAT')
            cfile.write_text(prelude+defines+'\n'+entry+'\n'+layout)
            arm=Path('C:/devkitPro/devkitARM/bin/arm-none-eabi-gcc.exe')
            self.assertTrue(arm.exists())
            result=subprocess.run([str(arm),'-mcpu=arm946e-s','-mthumb','-c',str(cfile),'-o',str(d/'layout.o')],capture_output=True)
            self.assertEqual(result.returncode,0,result.stderr.decode())


if __name__ == '__main__':
    unittest.main()
