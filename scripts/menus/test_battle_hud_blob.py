"""HUD residency changes must preserve the previous source-derived OBJ bytes."""
import hashlib
from pathlib import Path
import struct
import subprocess
import sys
import shutil


def test_hud_blob_preserves_pixels(tmp_path):
    root = Path(__file__).resolve().parents[2]
    header, blob = tmp_path / "hud.inc", tmp_path / "hud.bin"
    subprocess.run([sys.executable, str(root / "scripts/menus/generate_battle_hud.py"),
                    "--output", str(header), "--binary-output", str(blob)], check=True)
    data = blob.read_bytes()
    assert struct.unpack_from("<II", data) == (0x31444842, 12416)
    assert len(data) == 12424
    # Before/after byte comparison against the prior five linked Gfx arrays.
    assert hashlib.sha256(data[8:10376]).hexdigest() == "7d0da7bfe2b26260afb84b74eb0aab9c821ce75d556847e5db4bdc853fc4436e"
    text = header.read_text()
    assert "kNdsBattleHudDamageGfx" not in text
    assert "NDS_BATTLE_HUD_BLOB_BYTES 12416u" in text
    assert "kNdsBattleHudScorePalette" in text
    assert "kNdsBattleHudDamageMetric" in text
    assert "kNdsBattleHudPortraitPalette" in text


def test_hud_upload_flushes_each_dma_chunk(tmp_path):
    root = Path(__file__).resolve().parents[2]
    text = (root / "src/nds/nds_battle_hud.c").read_text()
    body = text[text.index("static u16 *ndsBattleHudAlloc"):text.index("static u32 ndsBattleHudPrepare")]
    code = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int SpriteSize;
#define SpriteColorFormat_16Color 0
int oamSub; u32 flushed, copies; unsigned char memory[1024], visible[512];
u16 *oamAllocateGfx(void *o, int size, int format) { (void)o;(void)size;(void)format;return (u16*)memory; }
void DC_FlushRange(const void *p, u32 bytes) { assert(bytes<=512);memcpy(visible,p,bytes);flushed++; }
void dmaCopy(const void *p, void *dest, u32 bytes) {
    (void)p; assert(flushed==copies+1); memcpy(dest,visible,bytes); copies++;
}
''' + body + r'''
int main(void) {
    unsigned char expected[1024];
    for (u32 i=0;i<1024;i++) expected[i]=(unsigned char)(i*7+3);
    FILE *f=tmpfile(); assert(f); assert(fwrite(expected,1,1024,f)==1024); rewind(f);
    assert(ndsBattleHudAlloc(f,0,1024)==(u16*)memory);
    assert(copies==2 && memcmp(memory,expected,1024)==0);
    assert(ndsBattleHudAlloc(f,0,32)==NULL); /* Truncated payload is not uploaded. */
    assert(ndsBattleHudAlloc(f,0,7)==NULL); /* DMA alignment is enforced. */
    fclose(f); return 0;
}
'''
    c_file, executable = tmp_path / "upload.c", tmp_path / "upload.exe"
    c_file.write_text(code)
    compiler = shutil.which("gcc") or shutil.which("clang")
    assert compiler
    compiled = subprocess.run([compiler,"-std=c11",str(c_file),"-o",str(executable)],capture_output=True,text=True)
    assert compiled.returncode == 0, compiled.stderr
    result = subprocess.run([str(executable)],capture_output=True,text=True)
    assert result.returncode == 0, result.stderr
