# Ogre 3D bugs and quirks

Every bug, limitation or surprising behaviour of Ogre (14.6.0, built from source by `scripts/deps.ps1`) that TUMBU ran
into, with what we do about it. **Add every new one here** (CLAUDE.md says so), and **read this list whenever Ogre is
upgraded** (`$OgreVersion` in `scripts/deps.ps1`): a patch may no longer be needed, or no longer apply. `deps.ps1` prints
a warning when a patch no longer finds the code it patches.

None of these is reported upstream yet (OGRECave/ogre on GitHub). The three patched bugs are good candidates for issues
or pull requests (the terrain seam fix is one line); that is Jonathan's call, since it posts publicly under his account.

| # | What | Kind | Our answer |
|---|---|---|---|
| 1 | Shared parameters reach a pass one bind late on Direct3D 11 | bug | patched (`Repair-OgreSource`) |
| 2 | Terrain delta-data leak (1 MB per match) | bug | patched |
| 3 | Seams along the terrain's texture repeats | bug | patched (shader media) |
| 4 | An optimised-out uniform set from code is a fatal error | behaviour | keep uniforms in use |
| 5 | `GpuSharedParameters::setNamedConstant` counts floats, not float4s | API trap | pass float counts |
| 6 | No RG8 texture format on Direct3D 11 (silently converted) | limitation | upload as RGBA |
| 7 | `setSkyBox` silently replaces a material without a cube map | behaviour | our own sky box |
| 8 | A post-processing final pass reads zeros from the depth texture on Direct3D 11 | limitation | depth effects in earlier passes |
| 9 | The RTSS builds from a material's first fixed-function technique | design | `ProgrammableTechniqueResolver` |
| 10 | GPU skinning is chosen from pass 0 only; shadow casters are not checked | design | every robot pass skins |
| 11 | A plain `hlsl` program compiles at level 9_1 | default | `target vs_4_0` / `ps_4_0` |
| 12 | The terrain light map is baked from the terrain's own heights only | design | light map off |
| 13 | The terrain's RTSS shadow stage does not fit an integrated depth shadow map | limitation | grass without shadows until the grass remake |
| 14 | Direct3D 11 compute passes get one writable view (one mip) per texture | limitation | none needed yet |
| 15 | Strict resource groups: only global-pool groups search other groups | design | `Game` in the global pool |

## Patched in Ogre (temporary: remove when fixed upstream)

`Repair-OgreSource` in `scripts/deps.ps1` applies these to the Ogre source (each marked `// TUMBU patch`).

### 1. Shared parameters reach a pass one bind late on Direct3D 11

- Bug: in Ogre 14.6.0 (and on `master` as of 2026-10-09; searched the OGRECave/ogre issues and the forums),
  `D3D11RenderSystem::bindGpuProgramParameters` (`RenderSystems/Direct3D11/src/OgreD3D11RenderSystem.cpp`) uploads the
  pass's constants (`updateDefaultUniformBuffer`) **before** it copies the shared parameters into them
  (`params->_updateSharedParams()`). OpenGL copies first.
- Why it matters: our lighting values are a shared set (`TumbuLighting`). On Direct3D 11 a draw got the values of its
  pass's previous bind, so a value changed for one render never reached it: the robot preview (`PreviewLighting` in
  `GUI.cpp`, `Lighting::beginNeutralLighting`) kept the arena's night lighting.
- Our workaround: the patch inserts the copy before the upload; `.\scripts\deps.ps1 -Only ogre` applies it and rebuilds.
  Cost: none (bench). We kept the patch instead of moving `TumbuLighting` into a constant buffer bound on its own (which
  the bug does not hit), because that means rewriting how every shader declares the lighting values.
- Fixed upstream when: in the new Ogre's `bindGpuProgramParameters`, `_updateSharedParams()` runs before
  `updateDefaultUniformBuffer` (or an Ogre issue/commit says so).
- To remove: delete the D3D11 block of `Repair-OgreSource`, rebuild Ogre, and check on Direct3D 11 with
  `.\scripts\devtest.ps1 -GuiTour -Hour 22`: in `devtest-gui-pause-inventory.png` the preview robot must be white
  (neutral light), not night blue. Then delete this entry.
- With an unpatched Ogre that still has the bug, the preview needs its own fix: copy the shared values into the preview's
  passes right after setting and after restoring them (`_copySharedParams()`; commit 86720f7).

### 2. Terrain delta-data leak (1 MB per match)

- Bug: in Ogre 14.6.0 (and on `master` as of 2026-10-09), `Terrain::prepare(StreamSerialiser&)`
  (`Components/Terrain/src/OgreTerrain.cpp`) allocates `mDeltaData` a second time for version-1 terrain files (our 2011
  Ogitor page) without freeing the first buffer.
- Our workaround: the patch removes the second allocation.
- Fixed upstream when: the line after `// Load delta data` in the version-1 branch no longer allocates.
- To remove: delete the terrain block of `Repair-OgreSource`, rebuild Ogre, and run
  `.\scripts\devtest.ps1 -Cycles 12 -Renderer GL`: the heap must stay flat from match 2 on. Then delete this entry.

### 3. Seams along the terrain's texture repeats

- Bug: in Ogre 14.6.0 (and on `master` as of 2026-10-11; no issue found), `blendTerrainLayer` in
  `Media/Terrain/TerrainSurface.glsl` computes each layer's UV as `mod(uv0 * uvMul, 1.0)`. The UV jumps from 1 to 0 where
  the texture repeats, the GPU takes the mip level from that jump and samples the smallest mip (the texture's average
  colour), so a 1 to 2 pixel light seam runs along every repeat line of the layer, dashed where the layer's blend weight
  drops below 1/255 (the function skips such layers). Both renderers, any distance. Found 2026-10-11 in the sand patch
  beside the ring (`DevTest -hide=terrain` showed it was the terrain; a crack was ruled out because hiding the sky did
  not change its colour).
- Our workaround: the patch replaces it by `uv0 * uvMul` (the layer samplers wrap anyway), in the Ogre source and in the
  installed media (`$install\Media`), so no Ogre rebuild is needed: `build.ps1` stages `OgreMedia\` into `bin\`. Cost: none.
- Fixed upstream when: `TerrainSurface.glsl` no longer wraps the layer UV with `mod`, or samples with gradients of the
  unwrapped UV.
- To remove: delete the `TerrainSurface.glsl` block of `Repair-OgreSource`, reinstall Ogre's media, and check
  `.\scripts\devtest.ps1 -Bench 1 -Hour 13 -Camera "4,2.5,9,1.5,0,16" -VideoMode 1600x900`: no light dashed line runs
  across the sand. Then delete this entry. When the grass gets its own shader (IMPROVEMENT_IDEAS, "Grass remake"), this
  patch goes too.

## Worked around in our code

### 4. An optimised-out uniform set from code is a fatal error

A uniform the shader never reads in the end (only used in a branch a `#define` turned off, or multiplied by a constant 0)
is removed by the compiler and does not exist for Ogre. A `param_named` for it in a script only logs
`ScriptCompiler - invalid parameters ... Parameter called X does not exist`, but setting it from code
(`GpuProgramParameters::setNamedConstant`, as `Lighting::notifyMaterialRender` does for `flareSun`) throws
`InvalidParametersException`, which ends the game. Met while prototyping the bloom (a debug view that skipped the lens
flare, 2026-10-10). Keep every uniform the code sets in use, or call `setIgnoreMissingParams( true )` on those parameters.
Shared parameters (`TumbuLighting`) are safe: missing names are skipped.

### 5. `GpuSharedParameters::setNamedConstant` counts floats, not float4s

`GpuSharedParameters::setNamedConstant( name, const float*, count )` takes the number of floats, while the
`GpuProgramParameters` overload takes the number of float4s. Passing 20 for the 20 torch positions (`lampPos[20]`) lit
only the first 5 torches (2026-10-09).

### 6. No RG8 texture format on Direct3D 11

Ogre's Direct3D 11 render system has no RG8 format and silently converted SMAA's area texture, which gave dashed outlines
on Direct3D 11 only. `Lighting::createSMAATextures` uploads it as RGBA. A material script cannot name `PF_RG8` either.

### 7. `setSkyBox` silently replaces a material without a cube map

`SceneManager::setSkyBox` accepts only materials whose first texture unit is a cube map and swaps any other for its
default ("skybox material ... is not supported, defaulting"). Our painted sky is our own box (`Sky`, a ManualObject in
`RENDER_QUEUE_SKIES_EARLY`).

### 8. A post-processing final pass reads zeros from the depth texture on Direct3D 11

The compositor's final pass could not sample the scene's depth attachment (it read zeros on Direct3D 11). Every
depth-based effect (god rays, fog, SSAO) is computed in an earlier pass that can read it (`input 0 scene 1`), and the lens
flare decides whether the sun is hidden on the CPU with Bullet rays (`Lighting::updateSunVisibility`).

### 9. The RTSS builds from a material's first fixed-function technique

With the RTSS viewport scheme (`MSN_SHADERGEN`), the shader generator builds its technique from a material's first
*fixed-function* technique, which is our fallback, and ignores the programmable one. `ProgrammableTechniqueResolver` in
`BaseApplication.cpp` (a `MaterialManager::Listener` on `MSN_SHADERGEN`) returns the material's own programmable
technique first. Robots and the sky depend on it.

### 10. GPU skinning is chosen from pass 0 only

Ogre checks only pass 0 of the best technique to decide on hardware skinning, and never checks the shadow caster
material. A pass without a skinned vertex program draws the rest pose; with Ogre's default caster the robots' shadows
kept the rest pose. So every robot pass skins, and the robots have their own skinned `Tumbu/RobotShadowCaster`
(CLAUDE.md, "Robots are skinned on the GPU", for the other skinning rules).

### 11. A plain `hlsl` program compiles at level 9_1

Without a `target`, Direct3D 11 compiles HLSL at feature level 9_1 (64 instructions). Our programs set `target vs_4_0` /
`ps_4_0`.

### 12. The terrain light map is baked from the terrain's own heights only

Ogre bakes the terrain's light map once at load from the terrain heights alone (mountains, the ground under the
coliseum), not from meshes such as the coliseum, so it drew still, wrong shadows on the grass. It is off since
2026-10-06 (`DotSceneLoader::processTerrain`); the grass has no shadows until it gets its own shader.

### 13. The terrain's RTSS shadow stage does not fit an integrated depth shadow map

Rebuilding Ogre's terrain material after the shadow technique is set makes its shader generator add a shadow stage, but
that stage expects a different shadow setup from our integrated depth map (`SHADOWTYPE_TEXTURE_MODULATIVE_INTEGRATED`,
sampled by our shaders) and drew long dark wedges (2026-10-06). Our own grass shader must sample `shadowMap` like
`env_toon.frag` (`tumbuShadow`).

### 14. Direct3D 11 compute passes get one writable view per texture

Ogre 14.6 has compositor `compute` passes, but its Direct3D 11 render system creates one unordered-access view per
texture, for a single mip (`D3D11Texture::createShaderAccessPoint`). A single-pass downsampler (such as AMD FidelityFX
SPD for the bloom) would have to write separate textures, not the mips of one. Not needed yet (2026-10-11,
docs/HISTORY.md, "Mip-chain bloom").

### 15. Strict resource groups

Only groups in the global pool search other groups for resources. The robot shaders `#include <OgreUnifiedShader.h>` from
`OgreInternal`, so `BaseApplication::locateResources` creates `Game` in the global pool.
