"""Source-backed closure for the DamageFlyMDust native owner."""

from __future__ import annotations

import sys
import shutil
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts" / "3d_vfx"))

import generate_nds_native_damage_fly_mdust as gen  # noqa: E402


def test_damage_fly_mdust_native_source_packet() -> None:
    decoded = gen.decode()

    assert gen.MODEL_FILE.file_id == 83
    assert gen.ROOT == 0xCA58
    assert gen.FRAME_OFFSETS == (
        0xC178, 0xB970, 0xB168, 0xA960, 0xA158, 0x9950, 0x9148,
    )
    assert decoded["triangles"] == ((3, 2, 1), (0, 3, 1))
    assert decoded["vertices"] == (
        (62, -62, 0, 1024, 0, 0xFFFFFFFF),
        (-62, -62, 0, 0, 0, 0xFFFFFFFF),
        (-62, 62, 0, 0, 1024, 0xFFFFFFFF),
        (62, 62, 0, 1024, 1024, 0xFFFFFFFF),
    )

    generated = gen.render(decoded)
    assert gen.OUT.is_file(), "run generate_nds_native_damage_fly_mdust.py --emit"
    assert gen.OUT.read_text(encoding="utf-8") == generated
    assert gen.OUT_HEADER.read_text(encoding="utf-8") == gen.render_header()
    assert decoded["band_masks"] == (8, 15, 15, 15, 15, 15, 15)

    executor = (
        ROOT / "src/nds/nds_native_damage_fly_mdust.exec.inc"
    ).read_text(encoding="utf-8")
    assert "ndsRendererNativeApplyMaterial(material, stats, &state)" in executor
    assert "ndsRendererNativeTexturedQuadGraded(" in executor
    assert "material->current_image" in executor
    assert "material->block_image" not in executor


def test_actual_c_layers_preserve_rgb5_alpha5_and_runtime_word_lanes() -> None:
    """Every IA16 value plus every source texel must have disjoint coverage."""
    cc = shutil.which("gcc")
    assert cc, "host gcc is required for the production C conversion proof"
    model = gen.sm.load_o2r(gen.REPO, gen.MODEL_FILE)
    frames = b"".join(model.payload[o:o + gen.FRAME_BYTES]
                      for o in gen.FRAME_OFFSETS)
    source = r'''
#include <stdio.h>
#include <stdint.h>
#include "nds/nds_damage_fly_mdust_texture.h"
static int check(const uint8_t source[4]) {
    uint8_t swapped[4] = {source[3],source[2],source[1],source[0]};
    unsigned pixel, band;
    for (pixel=0; pixel<2; pixel++) {
        unsigned coverage=0, wanted_i=source[2*pixel]>>3;
        unsigned wanted_a=source[2*pixel+1]>>3;
        for (band=0; band<4; band++) {
            unsigned texel=ndsDamageFlyMDustLayerTexel(source,pixel,0,band);
            if (texel != ndsDamageFlyMDustLayerTexel(swapped,pixel,3,band)) return 1;
            if (texel>>3) {
                coverage++;
                if ((texel>>3)!=wanted_a || band*8+(texel&7)!=wanted_i) return 2;
            }
        }
        if (coverage != (wanted_a!=0)) return 3;
    }
    return 0;
}
int main(int argc,char **argv) {
    unsigned i,a; unsigned count=0; uint8_t bytes[4]; FILE *f;
    if(argc!=2) return 4;
    for(i=0;i<256;i++) for(a=0;a<256;a++) {
        bytes[0]=i; bytes[1]=a; bytes[2]=i^127; bytes[3]=a^63;
        if(check(bytes)) return 5;
    }
    f=fopen(argv[1],"rb"); if(!f) return 6;
    while(fread(bytes,1,4,f)==4) { if(check(bytes)) return 7; count+=2; }
    fclose(f);
    if(count!=7*32*32) return 8;
    puts("RGB5_ALPHA5_EXACT: 65536 IA16 values, 7168 source texels, both byte layouts");
    return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix="dust-host-", dir=ROOT / "builds") as temp:
        temp = Path(temp)
        cfile, exe, data = temp / "test.c", temp / "test.exe", temp / "source.bin"
        cfile.write_text(source, encoding="utf-8")
        data.write_bytes(frames)
        subprocess.run([cc, "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror",
                        "-I", str(ROOT / "include"), str(cfile), "-o", str(exe)],
                       check=True, capture_output=True, text=True)
        result = subprocess.run([str(exe), str(data)], check=True,
                                capture_output=True, text=True)
        assert "RGB5_ALPHA5_EXACT" in result.stdout
