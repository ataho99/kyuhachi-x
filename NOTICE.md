# Third-party notices

Kyuhachi X is distributed under GPL-2.0 (see `LICENSE`). It bundles or links the following.

| Component | License | Where |
|---|---|---|
| DOSBox-X (parts, modified) | GPL-2.0 | `core_patches/`, `core_added/`, pinned in `third_party/DOSBOX_X.lock` |
| SDL2 | zlib | linked |
| zlib | zlib | linked |
| libpng | libpng | linked |
| Neodgm (NeoDunggeunmo), © 2017-2021 Eunbin Jeong (Dalgona.) | SIL OFL 1.1 | source: `legacy/work_t52i/OFL_neodgm.txt` |
| built-in bitmap font (derived from Neodgm by the Kyuhachi team, embedded in the program as `src/gensei_embed.h`) | SIL OFL 1.1 | built in |
| Shinonome 16 dot Gothic (`shnmk16.bdf`, JIS X 0208), converted to `res/fonts/shinonome16_jp.fnt` | Public Domain | notice: `legacy/work_t52i/shinonome16_LICENSE.txt` |
| SDL_GameControllerDB (`gamecontrollerdb.txt`) | zlib | notice: `legacy/work_t48e4/LICENSE_gamecontrollerdb.txt` |
| Sharp-Shimmerless shader | CC0 | converted, built in |
| CRT power-on / power-off sound effects (`res/disk-noises/crt_on.wav`, `crt_off.wav`) — "TV_CRT_PAL_TurnOn.wav" (https://freesound.org/people/firefreak/sounds/65400/) and "TV_CRT_PAL_TurnOff.wav" (freesound 65399, https://freesound.org/people/firefreak/sounds/65399/) by firefreak, licensed CC BY 3.0 (https://creativecommons.org/licenses/by/3.0/). Modified by the Kyuhachi team: decoded from the freesound preview mp3 (not the original wav) and converted to 44.1 kHz mono 16-bit; cut to 1.5 s (65400, 0.060–1.560 s) and 0.6 s (65399, 0.968–1.568 s) with short fades; gain applied (power-on level matched to the existing disk noises, power-off kept at its original ratio) | CC BY 3.0 | `res/disk-noises/` |

The full license texts of the bundled fonts and the GPL are in the `LICENSE` file shipped with each package.

Not distributed (and never committed): game images, NEC BIOS / font ROMs, FREECG98, original game fonts, YM2608 rhythm samples.
- stb_image v2.32 (https://github.com/nothings/stb, Sean Barrett) — public domain / MIT. PNG decoding for user touch-marker images (src/ui/stb_image.h, unmodified).
