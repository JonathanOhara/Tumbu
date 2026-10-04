# SMAA lookup textures

`AreaTex.h` and `SearchTex.h` are the precomputed area and search textures of SMAA (Enhanced Subpixel Morphological
Antialiasing, Jimenez, Echevarria, Sousa and Gutierrez, 2012), copied unchanged from the reference repository
https://github.com/iryoku/smaa (commit 71c806a, MIT licence: `LICENSE.txt`). `Lighting.cpp` turns them into two
textures at start-up. The shader, `SMAA.hlsl`, is in `media/tumbu/shading/smaa/`.
