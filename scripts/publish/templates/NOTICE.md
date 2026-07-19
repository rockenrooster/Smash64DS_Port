# Notices and provenance

Smash 64 DS Port combines port-owned Nintendo DS backend code with source based
on the BattleShip Super Smash Bros. 64 decompilation project. The public
repository intentionally contains no Nintendo ROM, extracted archive, audio,
texture, model, or other ROM-derived game asset. Builders must provide their own
legally obtained game dump, and the build regenerates required data locally.

The decompiled game source originates with
[ssb-decomp-re](https://github.com/VetriTheRetri/ssb-decomp-re). At the pinned
revision used here, that upstream project does not publish an explicit license.
This project makes no copyright claim over that material. BattleShip's MIT grant
also does not extend to Nintendo or HAL Laboratory game code or data.

BattleShip's port-specific source is distributed under the following notice:

```text
MIT License

Copyright (c) 2026 JRickey and contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

BattleShip obtains libultraship from
[Kenix3/libultraship](https://github.com/Kenix3/libultraship), copyright (c)
2022 kenix3, and Torch from
[HarbourMasters/Torch](https://github.com/HarbourMasters/Torch), copyright (c)
2023 Lywx. Both projects publish their own MIT licenses, which accompany their
source when the build downloads them.

The Nintendo DS backend also draws architectural ideas from
[Hydr8gon's sm64 Nintendo DS port](https://github.com/Hydr8gon/sm64) and uses
the devkitPro toolchain, libnds, and calico. Refer to each upstream project for
its current terms and contributor notices.
