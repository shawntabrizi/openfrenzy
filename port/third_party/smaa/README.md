# SMAA

Enhanced Subpixel Morphological Antialiasing, by Jorge Jimenez, Jose I.
Echevarria, Belen Masia, Fernando Navarro and Diego Gutierrez, MIT licensed
(see `LICENSE`).

Copied from OpenCE (`ref/OpenCE` at 231708d, `port/third_party/smaa`).
Upstream: https://github.com/iryoku/smaa, commit
71c806a838bdd7d517df19192a20f0c61b3ca29d. `SMAA.hlsl` is copied
unchanged. `area_tex.zlib` and `search_tex.zlib` are the bytes of
`Textures/AreaTex.h` (160x560, two channels) and `Textures/SearchTex.h`
(64x16, one channel), compressed with zlib (OpenCE's README gives the
command).

`OPENFRENZY_DISPLAY_ANTI_ALIASING=smaa` antialiases the 3D view with it,
compiled as GLSL ES 3.00 (`SMAA_GLSL_3`, `src/d3d8/xgpu_post.c`). The build
embeds the three files in the game image with
`port/macos/tools/embed_smaa.py`, which inflates the textures, so the guest
needs no zlib. The app bundle has `LICENSE` as
`Contents/Resources/licenses/SMAA.txt`.
