# Ogre 3D bugs and quirks

Every bug, limitation or surprising behaviour of Ogre (14.6.0, built from source by `scripts/deps.ps1`) that TUMBU ran
into, with what we do about it. **Add every new one here** (CLAUDE.md says so), and **read this list whenever Ogre is
upgraded** (`$OgreVersion` in `scripts/deps.ps1`): a patch may no longer be needed, or no longer apply. `deps.ps1` prints
a warning when a patch no longer finds the code it patches.

None of these is reported upstream yet (OGRECave/ogre on GitHub). The patched bug is a good candidate for an issue or a
pull request; that is Jonathan's call, since it posts publicly under his account.

| # | What | Kind | Our answer |
|---|---|---|---|
| 1 | Shared parameters reach a pass one bind late on Direct3D 11 | bug | patched (`Repair-OgreSource`) |
| 2 | Terrain delta-data leak (1 MB per match) | bug | retired (no Ogre terrain since 2026-10-12) |
| 3 | Seams along the terrain's texture repeats | bug | retired |
| 4 | An optimised-out uniform set from code is a fatal error | behaviour | keep uniforms in use |
| 5 | `GpuSharedParameters::setNamedConstant` counts floats, not float4s | API trap | pass float counts |
| 6 | No RG8 texture format on Direct3D 11 (silently converted) | limitation | upload as RGBA |
| 7 | `setSkyBox` silently replaces a material without a cube map | behaviour | our own sky box |
| 8 | A post-processing final pass reads zeros from the depth texture on Direct3D 11 | limitation | depth effects in earlier passes |
| 9 | The RTSS builds from a material's first fixed-function technique | design | `ProgrammableTechniqueResolver` |
| 10 | GPU skinning is chosen from pass 0 only; shadow casters are not checked | design | every robot pass skins |
| 11 | A plain `hlsl` program compiles at level 9_1 | default | `target vs_4_0` / `ps_4_0` |
| 12 | The terrain light map is baked from the terrain's own heights only | design | retired |
| 13 | The terrain's RTSS shadow stage does not fit an integrated depth shadow map | limitation | retired |
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

### 14. Direct3D 11 compute passes get one writable view per texture

Ogre 14.6 has compositor `compute` passes, but its Direct3D 11 render system creates one unordered-access view per
texture, for a single mip (`D3D11Texture::createShaderAccessPoint`). A single-pass downsampler (such as AMD FidelityFX
SPD for the bloom) would have to write separate textures, not the mips of one. Not needed yet (2026-10-11,
docs/HISTORY.md, "Mip-chain bloom").

### 15. Strict resource groups

Only groups in the global pool search other groups for resources. The robot shaders `#include <OgreUnifiedShader.h>` from
`OgreInternal`, so `BaseApplication::locateResources` creates `Game` in the global pool.

## Retired: the Ogre terrain (2026-10-12)

The game no longer uses Ogre's Terrain and Paging components: the field around the ring is a mesh modelled in
`art/arena/Arena.blend` (docs/HISTORY.md, "Grass remake"), so these went with them. Kept here in case the terrain comes
back:

- **2. Terrain delta-data leak (1 MB per match):** `Terrain::prepare(StreamSerialiser&)` (Ogre 14.6.0) allocated
  `mDeltaData` a second time for version-1 terrain files (the 2011 Ogitor page) without freeing the first buffer. It was
  patched in `Repair-OgreSource` (the second allocation removed).
- **3. Seams along the terrain's texture repeats:** `blendTerrainLayer` in `Media/Terrain/TerrainSurface.glsl` computed
  each layer's UV as `mod(uv0 * uvMul, 1.0)`; the jump at each repeat made the GPU take the smallest mip there, a 1 to 2
  pixel seam of the texture's average colour along every repeat line. It was patched to `uv0 * uvMul`.
- **12. The terrain light map is baked from the terrain's own heights only** (not from meshes such as the coliseum), so
  it drew still, wrong shadows on the grass; it was turned off.
- **13. The terrain's RTSS shadow stage does not fit an integrated depth shadow map:** rebuilding the terrain material
  after the shadow technique is set added a shadow stage that drew long dark wedges.
- **The terrain did not draw its own heights** in the ditch around the ring: it drew the ground there higher than its
  height data (found 2026-10-12 while prototyping tall grass, whose short blades were buried; the cause was not
  confirmed, perhaps its level of detail). The Blender mesh made from the same heights matches them to the millimetre.
