# TUMBU – next steps and improvement ideas

Backlog collected after the lighting overhaul (phases 0–5: toon shading, shadows, glow/bloom, baked AO, god rays,
lens flare, contact shadows, fog, dust, SSAO) and the special-attack rework of 2026-10 (docs/SPECIAL_EFFECTS.md).
Each idea notes which reference game does it.

## Priority order of the open items (2026-10-04)

Visual track, most important first (impact on what the player sees, against the effort):

1. **SMAA anti-aliasing** (Rendering modernization 1) — **done (2026-10)**.
   Open SMAA follow-ups, small: a quality choice (Low / High) in Options; colour edge detection if edges that differ only
   in hue (a red part against a green one) stay jagged.
2. **Arena remake** (Art side: coliseum retexture with parallax occlusion and self-shadows, a new ground, BC7/BC5
   textures): the biggest visual gap, in every shot. Large. **Art direction decided (2026-10-06)**, see "Coliseum
   retexture" under Art side; in progress.
3. **Night arena lights** (idea 4): night fights look unplanned; builds on the energy lights.
4. **Own stylised sky** (Rendering modernization 3) — **done (2026-10-06)**: the painted toon sky replaced Caelum (CLAUDE.md
   "The sky"); the same sky on OpenGL, Caelum and its patches gone. **Folded into the arena remake as its last step** (2026-10-06): the "painted bands" style.
5. **Mip-chain bloom** (Rendering modernization 2): better glow on the ki attacks, the game's hook.
6. **Tune Jyn in real fights** (idea 9): no code, only play time.
7. **Robot art pass:** richer textures and toon-friendly normals (Art side).
8. **Khronos PBR Neutral tone mapping** (Rendering modernization 5): small; fold into the bloom work.
9. **Cloud shadows** (idea 5).
10. **Colour-grading tables per time of day** (idea 6).
11. **Linear workflow** (Rendering modernization 4): only with a full retune of the look.
12. **GTAO** (Rendering modernization 6) and the **smaller extras** (idea 7).
13. **OpenGL frame rate** (Rendering modernization 9): about 30 % behind Direct3D 11; only worth it if OpenGL becomes a
    target (a Linux build).

Separate tracks: **gameplay beyond the battle demo** (part shop or loadout screen) is the most important item for the game
itself (customization is its core), but it is design and code, not visuals; the **Windows installer** comes last, once the improvements are done (Jonathan, 2026-10-04).

## Ideas, in order of impact

1. **Special attacks light the scene** *(Genshin, Astral Chain)* — **done (2026-10)**
   - Energy balls (Jyn, punch, kick) and impacts cast coloured light on the floor, walls and robots: four energy
     lights passed to the toon shaders as shared parameters (`tumbuEnergyLights` in `TumbuToon.h`,
     docs/SPECIAL_EFFECTS.md).

2. **Robot "hero lighting"** *(Genshin)* — **done (2026-10)**
   - A soft toon fill light that follows the camera (above and to its right) and the rim kept on the shadow side, so
     robots read in the coliseum's shadow, at dusk and at night; faint where the sun already lights them.
   - Only the robot shader changed (`tumbuHeroLight` in `TumbuToon.h`, `hero*` keys in `lighting.object`); the arena
     renders the same. Test any robot with `devtest.ps1 -Hero robot005`.
   - Possible follow-ups: black parts stay black (the fill multiplies the albedo), so they read only through the rim
     and outline; a Genshin-style screen-space depth rim, or softer cast shadows on robots, would go further.

3. **Metallic highlights for the robots** *(Genshin)* — **done (2026-10)**
   - A toon sky reflection in flat bands plus one sharp sun streak along the parts (`tumbuMetal`), so robots read as
     painted metal and black armour finally shows its shape. Mock-up of the options:
     https://claude.ai/artifact/Peu3ZK7cPLMfa1MrhDPcZW (option C chosen).
   - One `$metal` value per part (the spec maps carry no metal information); more metal at higher tiers.
   - The streak uses the smooth surface normal (not the normal map), so it stays a clean stripe on grooved parts.
   - Possible follow-ups: painted metal masks in Blender if specific spots shine wrongly (engraved lines, emblems); the
     streak axis is world-vertical, so a raised arm gets it across rather than along.

4. **Night arena lights** *(Astral Chain, Genshin)*
   - Torches or stadium lamps on the coliseum: flickering warm light, emissive glow and bloom.
   - Night fights would look intentional instead of just moonlit and dark.
   - Needs the same scene-light support as idea 1.

5. **Cloud shadows** *(Breath of the Wild)*
   - A scrolling noise texture multiplied into the sun term (and the god rays), drifting with the wind.
   - Cheap, and makes the scene feel alive.

6. **Colour-grading look-up tables per time of day** *(BotW, Genshin)*
   - A 3D look-up table in the final pass, blended between keyframes in `lighting.object`.
   - Gives richer golden-hour and night moods than exposure and tint alone.

7. **Smaller extras**
   - A subtle floor reflection: planar or screen-space, Astral Chain style.
   - Hit effects: a short screen flash, chromatic aberration and a small screen shake. *(Flash and shake done in the
     special-attack rework; chromatic aberration not.)*
   - Grass and foliage sway in the wind (BotW).
   - Per-robot light direction and crisper two-tone shading options (Granblue Fantasy Versus).
   - Dust spawned only in sunlit air, so the beams carry more of it.

## Special-attack ideas (to test)

8. **Ki aura around the robot while Jyn charges** *(Dragon Ball Z, Saint Seiya cosmos)* — **done (2026-10)**: the
   "Soft" body aura (inflated back-face shell with rising tongues, motes and a light, attack colour, thinner at the
   head); see `docs/SPECIAL_EFFECTS.md`. The notes below were the options considered.
   - A burning energy aura around the charging robot, in the attack's colour, that grows with the Genki Dama and
     flares at the throw. Built as an effect layer in `effects.object` (`jyn_charge`), so setups can be swapped and
     compared with `devtest.ps1 -FxTest special:jyn` without code changes.
   - Setups to compare:
     - **Body aura (DBZ):** stretched flame tongues rising around the silhouette. Cheapest good version: a few
       camera-facing, vertically stretched billboards around the robot with a scrolling-noise flame shader (toon bands,
       white inner edge, ki-coloured body, dark underlay like the orbs). Better but costlier: a second, inflated pass of
       the robot meshes (the outline shader, pushed out along the normals) with the same flame shader, so the aura hugs
       the robot's real shape.
     - **Ground aura:** a ring of energy and wind on the floor around the feet (a flat ring billboard plus dust pushed
       outwards), like the ground pressure under a powering-up fighter. Reuses the shockwave ring and dust pieces.
     - **Both**, and a **calm version** (only a faint shimmer outline and rising motes): with the Genki Dama the energy
       comes from outside, so a strong DBZ body aura may compete with the ball; the calm one keeps the focus on it.
   - Things to decide in the test: whether the aura follows the robot's colour or the attack's, its intensity while
     walking, and that it never hides the robot's face (the eye flare is part of the charge).
   - Cost: a few dozen billboards per charging robot; the inflated-mesh variant draws each robot part a second time.

9. **Tune Jyn in real fights** (no code: `skill jyn` in `media/configuration/skills.object`)
   - Does `throwSpeed 25` leave enough time to guard or dodge? Is the auto-aim (nearest enemy within ~60 degrees in
     front) fair? Does the enemy's crimson Genki Dama (`enemyKiColour`) read well? Is the ~2 s gathering too slow?
   - Scripted tests (`devtest.ps1 -JynHit`, `-FxTest`) only check that it works, not how it feels.

## Rendering modernization (review of 2026-10-04)

Techniques in use that have a clear modern replacement, most valuable first:

1. **Anti-aliasing: none during a match.** — **done (2026-10): SMAA 1x** (High preset, luma edges) chained after the
   post-processing (`Tumbu/SMAA`, Options → Anti-aliasing, default on); robot outlines and the ring ropes are smooth, effects
   and bloom unchanged, stencil-limited weights pass, about 0.06 ms per frame at 1920x1080 on Direct3D 11. Before/after:
   `%USERPROFILE%\Tumbu\smaa\` and `smaa-stencil\`. Original notes: The window asks for FSAA, but the scene renders into the post-processing
   compositor's HDR texture, which has no MSAA, so robot edges and outlines are jagged (visible in every screenshot).
   Add **SMAA** (or FXAA as a first step) in the final pass. SMAA suits toon outlines; TAA would blur them and needs
   motion vectors. Tested: FSAA=4 in the start-up dialog (`ogre.cfg`) is accepted by the window but leaves the match's
   edges exactly as jagged; it only affects what is drawn straight to the window (the start screen, the GUI). SMAA does
   not collide with it: it would be our own option in Options, and the dialog's FSAA can stay at 1 (or be hidden).
2. **Bloom: one bright pass + two H/V blurs at quarter size** (pre-2014 style). Replace with the **downsample/upsample
   mip-chain bloom** (Jimenez, Call of Duty: Advanced Warfare, SIGGRAPH 2014; used by Unreal and Unity): wider, more
   natural falloff, stable without flicker on small bright pixels (eyes, sparks, the Genki Dama).
3. **Sky: Caelum** — **done (2026-10-06): our own painted toon sky** (CLAUDE.md "The sky"). Original notes (a 2008-era library): Direct3D 11 only (OpenGL falls back to a static skydome), Cg/HLSL shaders, and
   two source patches in `deps.ps1`. Replace with **our own stylised sky shader** (gradient bands, sun and moon discs,
   toon clouds) driven by `lighting.object`: one sky on both renderers, art-directable like Genshin's skies, and a
   dependency less.
4. **Lighting in gamma space** (`lighting.object` colours are display space; no sRGB conversion). The modern standard is
   a **linear workflow** (sRGB textures decoded, light added in linear, encoded at the end): energy lights, bloom and
   fog blend correctly. Big retune of every value, so only together with a larger look pass; many toon games accept
   gamma-space lighting.
5. **Tone mapping: a per-channel soft shoulder.** Bright saturated colours shift hue as their channels roll off at
   different rates (an orange highlight turns yellow). **Khronos PBR Neutral** (2024) keeps base colours 1:1 up to a
   point and compresses only highlights, hue-preserving: the same goal ("the art keeps its colours"), done better.
6. **SSAO: 12 spiral samples.** **GTAO** (ground-truth AO) gives more accurate contact darkening for a similar cost.
   Low priority: the arena and robots also have baked AO.
7. **Uncompressed TGA/PNG textures.** **Arena done (2026-10-06):** BC7 / BC5 / BC4 DDS made from the PNGs by `scripts/compress-arena-textures.ps1` (the arena's textures in video memory: about 241 MB to 49 MB; slightly faster (bench, low wall view, 1920x1080: D3D11 +1.9 %, OpenGL +1.7 %)); the robots still use TGA. Original notes: **BC7** (colour) and **BC5** (normal maps) DDS with mipmaps: about 4x less video
   memory and faster loading, no visible change. Do it with the arena remake's new textures.
8. **Robots skinned on the CPU** — **done (2026-10): skinned on the GPU** (`RobotSkinning.h`): Direct3D 11 fights went from
   596 to 1371 fps, and the D3D11 driver memory growth per match halved (1.7-2.3 MB to 0.7-0.8 MB). The OpenGL
   cost of the first version (the whole 192-row bone array uploaded per draw) is fixed by a 32-bone array (2026-10-05):
   OpenGL fights 859 → 937 fps, as fast as CPU skinning was.
   Before/after: `%USERPROFILE%\Tumbu\gpu-skinning\`. Original notes (found 2026-10-04 with `-bench`): the robot shaders
   had no hardware skinning, so every
   animating part is skinned on the CPU and re-uploaded each frame. On Direct3D 11 that costs **+0.63 ms per frame for one
   fighting enemy** (+0.02 ms on OpenGL), because Ogre's D3D11 shadow buffers are STAGING resources: a still scene runs at
   1400 fps, a fight at 700. **Hardware skinning** (`includes_skeletal_animation`, the bone matrices and blend indices /
   weights in `robot_toon`, `robot_outline`, `robot_aura` and a skinned shadow caster for the robots) removes the CPU work
   and the uploads on both renderers, and leaves room for more robots on screen. Medium effort; check with
   `bench.ps1 -Variants "fight=-BenchAI"` before/after and the robot before/after shots (poses must not change).

9. **OpenGL about 30 % slower than Direct3D 11** (not investigated yet; noted 2026-10-06).
   - Measured with `bench.ps1` on the same scene and window size (RTX 3070, Ogre 14.6): tiers view before the arena remake
     1443 vs 1008 fps at 1024x768 (-30 %), 1127 vs 837 at 1920x1080 (-26 %); robot fight after GPU skinning 1371 vs 937.
     Every arena-remake step changed both renderers by about the same amount, so the gap is older than it.
   - Where the time goes: frames are about 1 ms, mostly the CPU submitting draw calls. The bench's `render_ms` (from
     `frameStarted` to `frameRenderingQueued`) is about 1.18 ms on OpenGL against 0.91 ms on Direct3D 11 for the same
     scene: Ogre's GL3+ render system costs more per draw (uniforms uploaded per pass and hashed for its cache, more state
     changes) than its D3D11 one (constant buffers).
   - Things to try, measured with `bench.ps1 -Renderers GL`: count the draw calls and passes per frame (Ogre's frame
     stats, RenderDoc / Nsight); fewer draws for the static arena (one entity per material, static geometry); shared
     parameters through uniform buffers if Ogre's GL3+ supports it for our programs; compare with Ogre's own samples to see
     whether the gap is ours or the render system's.
   - Priority: low. With VSync or the 144 fps cap both renderers run far above the limit, and Direct3D 11 is the default.

Still modern, keep: inverted-hull outlines (Genshin, Guilty Gear), the shadow-map raymarched god rays, integrated
depth shadows with normal offset (a single map is right for an arena this small), the fixed-step physics.

## Art side (not lighting, but the biggest visual gaps)

- **Coliseum retexture with parallax occlusion and self-shadows** *(planned together, 2026-10)*
  - Today: the coliseum (walls, seating, the ground around the ring) is a flat colour with baked AO
    (`coliseum.material`); the ring mat (`gym_arena.png`) is near-white and dominates every shot. The arena shader
    (`env_toon.frag`) has no normal map. Texture UVs are already the first UV set and the AO bake has its own second
    set (`scripts/blender/arena_ao.py`), so new tiling textures fit without breaking the AO.
  - **Decided (2026-10-06)** after seven rounds of Blender mock-ups rendered from the game's cameras and lighting
    (page with every round and the final look: https://claude.ai/artifact/LC8UBRa14SbNG46ANuTAu7):
    - **Coliseum: sandstone with moss.** Big warm ashlar blocks (about 2.2 x 0.85 m, running bond, two ochre tones,
      flat lighter patches), darker recessed and bevelled joints; sparse flat two-tone toon moss low on the walls, on the
      ledges and in the joints. The moss is its own layer with a mask, so its amount can be tuned without baking the stone
      again. Rejected: grey castle stone (with grey tiles the ring and the walls read as one material), worn ancient,
      tournament (painted band), dark basalt.
    - **Ring: today's hexagon, posts and ropes kept** (no gameplay or physics change). The floor becomes grey tournament
      stone tiles (1.6 units, each tile one of four flat greys, +-12 %, a lighter border course, stone sides). Rejected:
      a navy canvas mat (blue robot004 disappears on it), a stone platform or a hybrid with an inlaid mat (no ropes means
      invisible walls or a ring-out rule), white tiles.
    - **Centre: a charcoal painted T** (the robots' chest emblem, about 5 units wide; the tile joints show through it)
      with a **red neon tube** just inside its edge.
    - **Ropes: red neon**, a deep saturated red (a white-hot core made it read pink). The ropes and the T tube glow only
      at dusk and night, faded in by a `lighting.object` keyframe value.
    - **Field** between the ring and the walls: the grass stays (Ogre terrain).
    - **Sky: our own "painted bands" sky** (hard colour bands, flat two-tone cumulus, a yellow sun disc with a smooth soft
      glow, crescent moon and stars), built as the last step of this pass; it replaces Caelum (Rendering modernization 3).
    - Contrast checked on white robot001, blue robot004 and black robot005 at 09:00, 13:00, 17:00, 19:00 and 22:00.
  - Goal: stone that reads as carved masonry from the chase camera's low angle, in the toon look: sunken joints, stones
    hiding each other, hard toon shadows in the joints (explainer and live demo:
    https://claude.ai/artifact/3Dr1UwKxTmSTafxekL48Py, option 4).
  - Steps:
    1. **Art direction first** (with mock-ups to choose from): stone type and colours (sandstone blocks, worn ancient
       arena or a cleaner tournament look), how the ring fits in (keep the wrestling ring and ropes? a darker, less white
       canvas mat or a stone fighting platform). Jonathan wants a **complete remake** of the arena ground (today's
       texture is low resolution) and to decide ring vs stone platform from **side-by-side mock-ups** at that point.
    2. **Blender, shapes:** model the big forms the texture cannot fake (block edges, step lips, arches, bevels) in
       `art/arena/Arena.blend`; keep the AO bake script working (`clean_mesh`, the AO UV set). **Done (2026-10-06):**
       `scripts/blender/arena_shapes.py` (plinth course, chamfered hard edges, weighted normals; DEV_SETUP Part B); the
       windows already had reveals. No frame-rate cost (bench, D3D11 and OpenGL, 1024x768 and 1920x1080, inside the
       noise); before/after: `%USERPROFILE%\Tumbu\arena-shapes\`.
    3. **Blender, textures:** one or two tileable stone sets baked from modelled or sculpted stones, so the maps agree:
       colour, normal, **height** (the parallax needs it) and AO detail. Plus the new ring mat. Toon-friendly: flat
       colour areas, clear shapes, little photo noise. **Done (2026-10-06):** generated rather than baked from sculpts,
       by `scripts/arena-textures.ps1` (`scripts/blender/arena_textures.py`, Blender's numpy): every map of a set comes
       from one height field, so they agree by construction, and every set tiles; the moss is a field in the height
       map's G channel that the shader thresholds, so its amount is a material value.
    4. **Export:** tangents for `coliseum.mesh` / `arena.mesh` (blender2ogre option, through `bake-arena-ao.ps1`).
       **Done differently (2026-10-06):** no tangents in the meshes. The shader builds the tangent frame per pixel from
       screen-space derivatives (`tumbuTangentFrame`, TumbuStone.h), the same on Direct3D 11 and OpenGL. The meshes got
       world-scale texture UVs instead (`arena_shapes.py` version 3: walls unwrapped, each piece turned so "up" is +v and
       shifted so v = height / 4.8 m, so the courses run level all around; floors mapped from above), and the ring got
       its own floor and rope materials (`arenaFloorMaterial`, `arenaRopesMaterial`).
    5. **Shader (`env_toon.frag`):** normal map, then parallax occlusion (8–32 steps, depth per material) with
       toon-banded self-shadows towards the sun, faded to the plain normal map in the distance; switched per material
       (`Tumbu/EnvironmentToon` variables) and off on Low shadow/quality options. Values in the material and
       `lighting.object`. **Normal map, moss and the ring decal done (2026-10-06):** `Tumbu/EnvironmentStone` /
       `Tumbu/EnvironmentStoneFloor` (`env_stone_ps`, `env_stone_floor_ps`); before/after:
       `%USERPROFILE%\Tumbu\arena-stone\`; no frame-rate cost (bench, tiers view: D3D11 1143 vs 1151 fps at 1024x768, 880 vs 851 at 1920x1080; OpenGL 750 vs 769 and 808 vs 732; all inside the run-to-run spread); D3D11 driver memory +1.1 MB per match (CLAUDE.md).
       **Parallax occlusion with toon self-shadows done (2026-10-06):** `tumbuParallax` / `tumbuParallaxShadow`
       (TumbuStone.h): layers = `$stoneParams.y` (12 on the coliseum, 6 on the ring floor), doubled at grazing angles and
       halved head-on, then 4 halvings and a linear step (a single linear guess left layer stripes on the steep block
       edges); faded out by 30 units; the self-shadow is a 6-step march towards the sun, hard (toon), counted only for a
       stone at least 10 % of the depth higher (the faces' undulation shaded itself) and skipped where the shadow map
       already shades or the surface faces away. Before/after: `%USERPROFILE%\Tumbu\arena-parallax\`. Cost (bench,
       low wall view, against the step-5 build): 1024x768 inside the noise on both renderers; 1920x1080 D3D11 +0.05 ms
       (noise), OpenGL +0.18 ms (it was +0.41 ms with 16/32 layers and a longer shadow march).
    6. **Checks:** fixed low cameras (`-Camera`), all hours, D3D11 and OpenGL, frame rate with the stone filling the
       screen, `-cycles` memory.
    7. **Red neon (2026-10-06, done):** the ring ropes and the tube inside the T glow red at dusk and night (keyframe
       `neon` in `lighting.object`, `neonStrength`; CLAUDE.md "Arena neon"); the bloom makes the halo. Before/after:
       `%USERPROFILE%\Tumbu\arena-neon\` (19:00 and 22:00, plus 13:00 unchanged and D3D11 against OpenGL). Cost: none measurable (bench at 22:00, wide view: D3D11 and OpenGL, 1024x768 and 1920x1080, all inside the run-to-run spread).
    8. **BC7/BC5 textures and the low setting (2026-10-06, done):** the arena's textures are compressed DDS (BC7 colour and
       masks, BC5 normal and height + moss, BC4 AO; about 241 MB to 49 MB of video memory), made from the PNG sources by
       `scripts/compress-arena-textures.ps1` (texconv; `build.ps1` runs it, the DDS files are not in git). With Shadows
       off (the low setting) the stone drops its parallax and self-shadows and keeps the normal map (shared
       `detailParams`). Before/after: `%USERPROFILE%\Tumbu\arena-bc\`. Cost: slightly faster (bench, low wall view, 1920x1080: D3D11 +1.9 %, OpenGL +1.7 %).
    9. **The painted toon sky (2026-10-06, done):** replaces Caelum (CLAUDE.md "The sky"): colour bands, the yellow sun with
       a soft glow, crescent moon and stars, flat two-tone cumulus with Sky quality High; keyframe colours in
       `lighting.object`. Same cost as Caelum; Caelum, its media and its `deps.ps1` patches are gone. Before/after (Caelum
       against the painted sky at 09:00 to 22:00, three cameras, plus Direct3D 11 against OpenGL):
       `%USERPROFILE%\Tumbu\toon-sky\`.
       Possible follow-up: let the neon light the floor and the robots near the ropes (a strip light in the arena
       shader; the energy lights are limited to four), or a slow pulse.
  - Note: the parallax suits the stone, not the canvas ring mat (no depth to show there). The sun's shadow map and the
    contact shadows still land on the flat surface; only the stones' own shadows follow the depth.
- **The robot textures are plain.** A colour and material pass per robot would fit the anime style.
- Both are Blender work: `art/arena/Arena.blend` and `art/robots/` are the sources. Follow the Blender-first
  rule in CLAUDE.md.
- **Toon-friendly normals for the robots** *(Guilty Gear Xrd, Genshin)*: the modern anime-game answer to "better
  normal mapping" is not more surface detail but **cleaner shading**. Edit the robots' vertex normals in Blender (Data
  Transfer from a smooth proxy shape, Normal Edit / Weighted Normal modifiers), so the toon bands fall in clean,
  deliberate shapes instead of following every polygon; then bake the 2011 normal maps again from a high-poly version
  (bevels, panel lines) for the rounder, later style. Every lighting feature gains from it (sun bands, hero fill, metal
  reflection, streak). An art pass per robot; needs the 14 animations kept (DEV_SETUP Part B).
- **Not recommended for the robots: parallax mapping and tessellation.** Parallax occlusion mapping fakes depth from
  a height map; tessellation adds real geometry from a displacement map (Ogre 14 supports it on Direct3D 11 / OpenGL 4).
  Both suit detailed realistic surfaces (stone, bricks, terrain), but the robots are flat-shaded toon armour with no
  height maps, and on a slim, moving silhouette the effect would barely show. Today, extra detail simply goes into the
  mesh (GPUs draw millions of triangles; UE5's Nanite is the extreme of that). Parallax occlusion belongs to the
  coliseum stone instead: see "Coliseum retexture" above.

## Beyond the visuals

- **Windows installer**: `Tumbu.nsi` still has the 2011 x86 layout (DLL names, folders, vcredist). Update it for the
  x64 Ogre 14 build and its staged runtime (`cmake/StageRuntime.cmake`); the NSIS steps are in DEV_SETUP Part B.
- **Gameplay beyond the battle demo**: customization is the core of the game (ART_AND_DESIGN), but today parts are only
  won from defeated enemies. A part shop or loadout screen to build the robot between fights (buy parts, equip and
  compare them), then more arenas or a mode beyond the 16 fixed fights.
- **One shared robot skeleton (a rig file as the source)**: found while building the robot export (2026-10-05).
  - Today: each `art/robots/robot00N.blend` has its own armature. All five share the same 50-bone hierarchy, bone
    names and 14 actions, but the proportions differ by up to 8 cm and a few action tracks differ. The game gets 25
    skeleton files, one per part, each a copy of its robot's armature moved to the part's origin.
  - Idea: `art/robots/rig.blend` holds only the armature and the 14 actions, and is the source for the skeleton and
    the animations. Robot files either **link** it (Blender library linking with a library override, so a robot can
    adjust its proportions while the actions stay shared) or **append** it once as the starting point for a new robot
    (a template to clone).
  - In Ogre:
    - **One skeleton per robot** instead of one per part. The parts share one skeleton instance
      (`Entity::shareSkeletonInstanceWith`, already used for the ki aura shell), so each robot updates its animation
      once instead of five times.
    - **Optionally one animation set for every robot:** `Skeleton::addLinkedSkeletonAnimationSource` plays the
      animations of a master skeleton on any skeleton with the same bone names (bone offsets scaled). The animations
      are then authored once.
  - What it touches:
    - Parts are swapped between robots, so robots with different proportions need attach points that still line up.
    - The per-part re-centring and `robot00N.object` would change, along with `Part` / `Robot`.
    - The export (`scripts/export-robots.ps1`) would write one skeleton per robot.
  - Gain: animate once; new robots start from the rig; mixed robots move consistently; fewer files.
