# TUMBU – done work: what was built, measured and decided

The record of finished work, so CLAUDE.md can stay about how things are now and docs/IMPROVEMENT_IDEAS.md about what is
still open. Each entry keeps the decisions, the measurements and the alternatives that were tried, so a later change
does not repeat them. Newest topics are added at the end of their section. The 2011 → Ogre 14 port itself is in
docs/MODERNIZATION_PLAN.md, the special-attack rework in docs/SPECIAL_EFFECTS.md, Ogre's bugs in docs/OGRE_BUGS.md.

Frame-rate numbers are from `scripts/bench.ps1` on the development PC (RTX 3070), comparing builds run after run in one
session; they only mean something against the other variant of the same run.

## Lighting and look

- **Lighting overhaul, phases 0–5** (2026-09/10): toon shading, integrated depth shadows, emissive glow and bloom, baked
  AO, god rays, lens flare, contact shadows, distance fog, dust, SSAO (how each works: CLAUDE.md, "Rendering pitfalls").
- **Special attacks light the scene** (2026-10, idea from Genshin / Astral Chain): energy balls and impacts cast coloured
  light on the floor, walls and robots, four energy lights as shared parameters (`tumbuEnergyLights`).
- **Hero lighting** (2026-10, Genshin style): a soft toon fill that follows the camera, and the rim kept on the shadow
  side, so robots read in the coliseum's shadow, at dusk and at night; faint where the sun already lights them. Only the
  robot shader changed (`tumbuHeroLight`, `hero*` keys in `lighting.object`).
- **Robot metal** (2026-10): a toon sky reflection in flat bands plus one sharp sun streak along the parts (`tumbuMetal`),
  option C of the mock-ups (https://claude.ai/artifact/Peu3ZK7cPLMfa1MrhDPcZW). One `$metal` value per part (the 2011
  spec maps carry no metal information: they are greyscale copies of the colour texture). The streak uses the smooth
  surface normal, so it stays a clean stripe on grooved parts. Softened in four rounds (2026-10-04) to the values now in
  the `robotNNN.material` files.
- **Robots skinned on the GPU** (2026-10-04/05). Found with `-bench`: the robot shaders had no hardware skinning, so every
  animating part was skinned on the CPU and re-uploaded each frame; on Direct3D 11 that went through STAGING "shadow
  buffers" and one fighting enemy cost +0.63 ms per frame (+0.02 ms on OpenGL): a still scene ran at 1400 fps, a fight at
  700. With `RobotSkinning.h` Direct3D 11 fights went from 596 to 1371 fps (1.68 → 0.73 ms, 1024x768), the same as a still
  scene, and the Direct3D 11 driver memory growth per match halved (1.7–2.3 to 0.7–0.8 MB). The first version uploaded a
  192-row bone array per draw and made OpenGL 0.08 ms slower than CPU skinning (859 against 920 fps); the 32-bone array
  (96 rows, 2026-10-05) made it as fast or faster (937 fps) and steadier.
- **SMAA 1x anti-aliasing** (2026-10). Before it a match had no anti-aliasing at all: the window asks for FSAA, but the
  scene renders into the post-processing HDR texture, which has no MSAA. Tested: FSAA=4 in the start-up dialog is accepted
  by the window but leaves the match's edges exactly as jagged; it only affects what is drawn straight to the window (the
  start screen, the GUI). SMAA was chosen over TAA (blurs toon outlines, needs motion vectors) and FXAA. The stencil that
  limits the blending-weight pass to edges was proven by clearing the weights to 0.5 as a test (46 % of the pixels
  changed on Direct3D 11, 27 % on OpenGL; normal runs differ by ~7 %). Cost at 1920x1080: Direct3D 11 +0.095 ms without
  the stencil, +0.060 ms with it; OpenGL +0.015 / +0.031 ms, both inside the noise; nothing at 1024x768.
- **OpenGL god rays fixed** (2026-10-06): an old "OpenGL stores the depth texture upside down" flip mirrored every view
  ray (same length, so distances and fog looked right), and on OpenGL the god rays marched into the sunlit air above the
  walls, washing the view towards the sun white. The flip is gone (god rays and SSAO).
- **Dust density** (2026-10-06): about 1200 motes, the middle between the first 2000 (too busy) and 350 (too sparse).
- **The terrain's light map turned off** (2026-10-06): it drew still, wrong shadows on the grass (docs/OGRE_BUGS.md, 12).
- **Mip-chain bloom** (2026-10-10/11), replacing a bright pass to quarter size and two H+V blurs:
  - Chosen from a prototype rendered in game (https://claude.ai/artifact/DwgAko9KuG5ZwW15Eh8x1r): "Balanced" (equal
    weights on six levels) over "tight" (1 0.7 0.45 0.25 0.12 0.06) and "wide anime glow" (0.4 0.5 0.7 1 1.3 1.6).
  - Against the old bloom, hot cores keep their shape (the punch and Genki Dama centres, the three neon ropes, which used
    to melt into one red band), and the glow crawls about 4 times less while the camera pans (bloom-only frame strip of
    the T's neon tube: 1.25 → 0.30 %). The energy effects needed no retuning (checked 2026-10-11).
  - Cost: +0.075 ms at 1920x1080 against the old bloom on both renderers, nothing at 1024x768; the budget was +0.05 ms.
    Five levels, a 4-tap first step and R11G11B10 targets were no cheaper on Direct3D 11. A tent filter in the final pass
    cost 0.04 ms and looked the same as one bilinear sample.
  - Where the cost goes (2026-10-11, builds with the chain removed, with its 11 passes writing black without reading
    anything, and with 3 levels): the whole chain costs about 0.10 ms on Direct3D 11 and 0.14 ms on OpenGL at 1920x1080,
    and 0.05 to 0.06 ms of that is the passes themselves (some 5 microseconds each, however small); the sampling is the
    rest, mostly the two half-size passes. Three levels instead of six save only 0.015 / 0.04 ms. Ways to lower it are
    in docs/IMPROVEMENT_IDEAS.md; none is worth it at 1.4 % of a 144 fps frame.
- **Khronos PBR Neutral tone mapping tried** (2026-10-10), softShoulder kept: on our display-space image PBR Neutral made
  every scene about 10 % darker, nearly doubled the near-black pixels at night (its toe acts on visible darks: robot004's
  navy armour sank), and its highlight whitening turned the deep-red neon pink (rope 238,12,7 became 239,91,91). Without
  the whitening the neon stayed red but the toe still darkened. Jonathan liked both looks; revisit with a linear workflow.
- **Robot texture filtering checked** (2026-10-11): anisotropic filtering on the robot texture units changed nothing
  (identical still shots, same frame-strip crawl); the robots' fine lines are not texture aliasing. Not kept.

## Arena remake (2026-10-06 to 10-09)

- **Art direction** decided after seven rounds of Blender mock-ups rendered from the game's cameras and lighting
  (https://claude.ai/artifact/LC8UBRa14SbNG46ANuTAu7):
  - Coliseum: sandstone with moss (big warm ashlar blocks, running bond, two ochre tones, recessed bevelled joints;
    sparse two-tone toon moss low on the walls, on the ledges and in the joints, its own masked layer). Rejected: grey
    castle stone (with grey tiles the ring and the walls read as one material), worn ancient, tournament (painted band),
    dark basalt.
  - Ring: the hexagon, posts and ropes kept (no gameplay or physics change); the floor became grey tournament stone tiles
    (1.6 units, four flat greys, a lighter border course). Rejected: a navy canvas mat (blue robot004 disappears on it), a
    stone platform or a hybrid with an inlaid mat (no ropes means invisible walls or a ring-out rule), white tiles.
  - Centre: a charcoal painted T (the robots' chest emblem) with a red neon tube inside its edge; ropes in red neon (a
    white-hot core read pink), glowing at dusk and night.
  - Field: the grass stays (Ogre terrain). Sky: "painted bands".
  - Contrast checked on white robot001, blue robot004 and black robot005 at 09, 13, 17, 19 and 22 h.
  - Stone look: carved masonry from the chase camera's low angle (explainer: https://claude.ai/artifact/3Dr1UwKxTmSTafxekL48Py,
    option 4).
- **Shapes** (`scripts/blender/arena_shapes.py`): plinth course, chamfered hard edges, weighted normals; no frame-rate cost.
- **Textures** generated rather than baked from sculpts (`scripts/arena-textures.ps1`, Blender's numpy): every map of a
  set comes from one height field, so they agree by construction, and every set tiles; the moss is a field in the height
  map's G channel that the shader thresholds.
- **No mesh tangents**: the shader builds the tangent frame per pixel (`tumbuTangentFrame`), the same on both renderers;
  the meshes got world-scale texture UVs instead (walls unwrapped so v = height / 4.8 m and the courses run level all
  around; floors mapped from above), and the ring its own floor and rope materials. The arena meshes had carried unused
  tangents (blender2ogre writes them by default) until 2026-10-09: 27 % of their size.
- **Stone shader**: normal map, moss and the ring decal, no frame-rate cost (Direct3D 11 driver memory +1.1 MB per match
  at the time). Then parallax occlusion with hard toon self-shadows: a single linear step left layer stripes on the steep
  block edges, so the search ends with 4 halvings; the self-shadow only counts a stone at least 10 % of the depth higher
  (the faces' undulation shaded itself). Cost at 1920x1080, low wall view: Direct3D 11 +0.05 ms (noise), OpenGL +0.18 ms
  (+0.41 ms with 16/32 layers and a longer shadow march).
- **Red neon**: no measurable cost (22:00, wide view, both renderers and sizes).
- **BC7/BC5/BC4 textures** (`scripts/compress-arena-textures.ps1`): the arena's textures in video memory from about 241 MB
  to 49 MB, slightly faster (low wall view, 1920x1080: Direct3D 11 +1.9 %, OpenGL +1.7 %). texconv converted the
  sRGB-tagged PNGs to linear until it got `--ignore-srgb` (everything darker, the moss gone).
- **The painted toon sky** replaced Caelum, a 2008-era library that was Direct3D 11 only (OpenGL fell back to a static
  skydome), used Cg/HLSL shaders and needed two source patches in `deps.ps1`. Same cost as Caelum (the sky filling the
  view, 1920x1080: Direct3D 11 +0.006 ms, OpenGL -0.011 ms). The 2011 god-ray march over sky pixels washed the bands out,
  so it is short now.
- **blender2ogre's sticky settings** turned the arena on its side once (2026-10-06): the export had relied on the default
  `SWAP_AXIS`, and a robot export had saved another; every export script now sets it.
- **The terrain's missing layer** (2026-10-06): the 2011 page had a round patch beside the ring painted with Ogitor's own
  `city_6` rock, never shipped, which drew black; the page now names the grass textures for that layer (the `.ogt` chunk
  lengths and checksums were fixed by hand).

## Night lights (2026-10-09)

- Direction from Blender mock-ups rendered with the game's cameras (https://claude.ai/artifact/4X5cxLXTGZuXMQucDSryq8):
  wall torches in iron brackets (chosen; then 25 % fewer: 17, one every 20 degrees), against stadium lamp posts (they
  turned the night into day and stood in the camera's way), braziers on the ledges (lit the upper wall, not the fight)
  and lanterns in the windows (lit almost nothing). Neon light "soft" (strong turned the ring pink), mood "balanced" (the
  night's moon and ambient x0.75, dusk x0.85).
- Fixed on the way: the flame's height from its UVs read the wrong way round (blender2ogre writes 1 - v), so the flame's
  base moved against the cup; the iron brackets lit by their own flame turned the colour of the wall; a mirrored bracket
  matrix built the plate inside-out (z-fighting); the neon first had the torches' 4 toon bands and drew rings around the
  ends of the T; the neon's light was too faint under the ropes at first (`neonLight 2`, `neonLightReach 2`).
- The inventory preview took the arena's lighting (the T's neon turned its feet pink at night): `PreviewLighting` now
  sets neutral values around each preview render, which exposed Ogre's late shared parameters on Direct3D 11
  (docs/OGRE_BUGS.md, 1).
- Cost (22:00, chase view, against the build before): 1920x1080 Direct3D 11 +0.070 ms, OpenGL +0.045 ms; 1024x768 inside
  the noise; by day about +0.02 / +0.03 ms. Memory unchanged.

## Grass remake (2026-10-12)

- Direction from two decision pages with in-game mock-ups (https://claude.ai/artifact/XynMB45r7suQkAyFBA4V55, looks and
  ways to get our shader on the ground; https://claude.ai/artifact/FYxBgrmU9PwroZquVRcVsr, tall grass and the Blender
  proof), built in a throwaway prototype. Jonathan's picks: the "wild meadow" ground (over a painted lawn and a
  tournament turf), toon blades for the tall grass (over thin dense blades and alpha cards), the density as prototyped,
  the field as a mesh from Blender (Blender the single source of all geometry: no Ogitor), the earth's stones at 3/4 of
  the way from the first set to a softened one (the first stood out as white circles, the softened went too far), and
  grass that the robots push aside even though they stay in the ring today (they may leave it later).
- How the games do tall grass (from the research page): generated, not modelled. Artists paint where it grows and how
  tall; code builds the blades at load; the vertex shader bends them (wind, characters). Ghost of Tsushima (GDC 2021) is
  the only documented one (Bezier blades rebuilt on the GPU, about 83 000 on screen); Breath of the Wild, Xenoblade 3,
  Genshin and Relink were analysed from footage. Xenoblade's only public note: a temporal filter against its grass
  flickering.
- Ways to get our shader on the ground that were weighed: a custom `TerrainMaterialGenerator` for the Ogre terrain
  (prototyped: works, `-0.04 ms`), the RTSS terrain stage (rejected, docs/OGRE_BUGS.md retired 13), and the field as a
  mesh from Blender (chosen). The prototype found that the Ogre terrain drew the ditch around the ring higher than its own
  heights (it buried short blades); the Blender mesh matches the heights to the millimetre.
- **Step 1, the field from Blender:** `arena_field.py` read the 2011 page (heights and blend maps) and built the `field`
  object in `Arena.blend` (513 x 513 grid, hills beyond 34 units decimated to 12 %: 263 k to 31 k vertices; 1.4 MB mesh
  instead of the 16.5 MB page), the first field map (earth band from 8.3 to 13.5 units with a wobbling outer edge, the
  2011 desert and ice layers on the hills, tall grass rising from 14 to 22 units) and a top-down UV map; the AO bake (2048,
  2.6 minutes) adds the walls' shade on the ground. The meadow textures moved from the prototype's Pillow script into
  `arena_textures.py` (numpy only). The game lost its Ogre terrain code, the Terrain and Paging components, the old layer
  textures and the Ogitor project file; the physics became a trimesh of the same mesh. `deps.ps1` dropped the two terrain
  patches.
- Cost of step 1 (1920x1080, the standard robot view, against the build before): Direct3D 11 -0.070 ms (729 to 768 fps),
  OpenGL inside the noise (+0.009 +/- 0.014 ms). The prototype had shown that the old terrain's cost was its shading per
  pixel, not its triangles. Memory (40 matches each): flat on both renderers; the levels about 2 MB higher (OpenGL 19.9 MB
  after match 1 instead of 18; the Direct3D 11 plateau 87 to 92 MB instead of 81 to 88), the field's triangle-mesh
  physics (62 k triangles with their BVH, about 3 MB) against the old 1 MB heightfield.
- **Step 2, the toon tall grass** (`Grass`): the prototype's toon blades moved into the game, placed from the field map
  instead of radius rules (the map's first B channel bakes those rules in: short near the earth band, full height from
  22 units, none beyond 30). 45 k blades, 316 k vertices in 54 chunks of 8 m, built in about 250 ms at match load
  (144 k tries: 15 k on the earth or rock, 54 k under the ring, walls and seats). The robots' push radius went from the
  prototype's 0.9 to 1.2 units from the robot's centre: at 0.9 the parting hid behind the robot's own legs. Checked with
  the hero standing in the grass (DevTest `-heropos`), push on against push off.
- Cost (1920x1080, against the same build with `-hide=grass`): the standard robot view Direct3D 11 +0.147 ms, OpenGL
  +0.176 ms; the field view (grass filling the screen, the worst case) +0.239 / +0.208 ms. The budget was 0.3 ms.
- Shimmer (the risk named on the decision page; we have SMAA, no temporal filter): frame strips of the field view at
  13:00 and 22:00, the camera panning a third of a pixel per frame. Crawl 2.11 % with the grass against 2.54 % for the
  bare ground (22:00: 2.52 / 3.04 %), and 1.55 % with the camera still (the wind alone). The blades crawl less than the
  ground's texture under them, so no anti-shimmer work was needed.
- Memory (40 matches each): flat on both renderers; after match 1 OpenGL 21.2 MB (+1.3 MB) and Direct3D 11 40 MB
  (+6 MB, its driver's copies of the 17 MB of blade vertices), the Direct3D 11 plateau unchanged (88 to 92 MB).
- **Snow on the hill tops** (2026-10-13, Jonathan: the tops looked like a white version of the earth): the field map's G
  (the 2011 "ice" layer) drew `meadow_rock_col`, grey Voronoi cells like the earth's. It now draws snow
  (`meadow_snow_col`: near white, wind drifts in a cool shade with a bright crest, no cells) with a thin shaded rim along
  its lower edge, and the hills' brown earth only where the slope passes about 66 degrees (first a darker grey rock;
  Jonathan: under the snow should be the brown earth, so `meadow_rock_col` went). The
  peaks are steep (median 59 degrees inside the snow); at 0.6 (about 60 degrees) with more noise the rock covered too
  much and its round holes read as a pair of eyes on the middle peak. Cost on the peak view at 1920x1080: Direct3D 11
  +0.009 +/- 0.013 ms, OpenGL -0.008 +/- 0.030 ms, both not significant. Memory (40 matches): flat on both renderers (OpenGL 21.5 MB, the
  Direct3D 11 plateau 87 to 93 MB).
- **Flowers, a first look** (2026-10-13, Jonathan, after Xenoblade 3's meadows: "a very small number, variations, a few
  around the ring, to validate the look"). The research first: in the games that document it the flowers are not made by
  the blade generator but are hand-made models placed by the same rules (Ghost of Tsushima: GPU-instanced flowers and
  pampas grass in its grass tiles; Horizon Zero Dawn: run-time placement from density maps); Xenoblade 3 has no public
  talk, its screenshots show the same split. Five kinds made in Blender by a script (`arena_flowers.py`), colours as
  vertex colours so they can be painted by hand: daisy, poppy, lupine, forget-me-not, marigold. Ten clumps of 3 to 6,
  one kind each, 14.5 to 18 units from the centre (35 flowers in 8 clumps: two clumps found no ground in the grass).
  - At real flower sizes (0.35 to 0.6 m, 5 mm stems) they were lost among the toon blades (up to 0.9 m and wide): heads
    35 % bigger and stems 1.6x thicker in Blender, planted 1.4 to 1.8x larger (0.5 to 1.1 m).
  - The blade layout stays exactly the same (45,204 blades): the ground test became a shared lambda that draws its random
    number at the same point.
  - Cost on a close view of a clump at 1920x1080, flowers hidden against shown: Direct3D 11 +0.009 +/- 0.034 ms, OpenGL
    +0.005 +/- 0.011 ms, both not significant. Memory (40 matches): flat on both renderers; OpenGL 21.6 MB after match 1
    (about 0.6 MB above the run before the flowers in the same session: their material and programs, made once), the
    Direct3D 11 plateau 91 MB, second half +54 KB per match.
- **The grass extras** (2026-10-13, Jonathan: reeds, a shockwave across the field, burnt grass, all three).
  - **Reeds:** pampas grass and cattails made like the flowers (the same script, file and code, a second plant set).
    The first pampas plumes closed into spindles, then opened into lily-like stars (the feathers curled towards their
    own spread); they read as pampas once all the feathers of a plume flag to the side the stem leans to. Out near the
    walls, where the grass is 0.9 m tall, clumps of 1.3 to 2.3 m were lost again: planted at 2 to 3 m, 2 to 3 per clump.
    Only 3 of 8 clumps found ground between 21 and 27 units (the walls and seats cover most of that band), so a clump
    may now retry elsewhere (`retries`, 0 for the flowers so their layout stayed). Cost on the wide view at 1920x1080,
    reeds hidden against shown: Direct3D 11 -0.008 +/- 0.008 ms, OpenGL +0.011 +/- 0.054 ms, both not significant.
    Memory (40 matches on each renderer): flat.
  - **Shockwave and burnt grass:** a `grass` layer in `effects.object` (`jyn_impact`: a strong wave across the whole field
    and a burnt patch of 3.2 units; `blast_hit`: a small ripple), sent to the grass, flower and field shaders as shared
    parameters like the energy lights. A frame strip could not show the wave: saving each frame takes longer than a
    frame, so the game jumped ahead between them (the wave and the embers were gone a few frames in); one run per moment
    (0.15 / 0.35 / 0.6 / 0.9 s after the impact) shows it. Cost on the field view at 1920x1080, against the build
    before: OpenGL -0.028 +/- 0.071 ms; Direct3D 11 first +0.032 +/- 0.020 ms, because every blade vertex and field
    pixel walked the eight burn slots even with none burnt; with the count of patches passed (`grassWaveWidth.z`, the
    loop stops there) +0.014 +/- 0.017 ms, not significant. Memory (40 matches on Direct3D 11, 20 on OpenGL): flat.

## Memory

- The Direct3D 11 heap growth per match is an NVIDIA driver pool tied to the post-processing compositor, not a leak: it
  rises for about 18 matches and levels off (40-match runs, 2026-10-09). History of the rate: +1.7 to 2.3 MB per match
  (12 matches) before the GPU skinning, +0.7 to 1.1 after it (2026-10-06); the night lights changed nothing (2026-10-09);
  the mip-chain bloom (11 render targets instead of 2, 2026-10-11): plateau 81 to 88 MB, second half -0.2 MB per match,
  OpenGL +70 KB.
- The Ogre terrain leaked 1 MB per match (docs/OGRE_BUGS.md, 2).
- **The OpenGL heap level** (2026-10-11): it measured 16.7 MB on 2026-10-09 and 17.9 to 18.0 MB on 2026-10-11. The
  2026-10-09 build, rebuilt and alternated with the current one, also measured 17.9, so the game is not the cause. Ruled
  out: the window size (it adds 640x480 17.9 MB, 1024x768 18.1, 1600x900 18.4), the driver (installed 2026-10-06), a
  reboot (17.93 MB before and after one), Windows updates, the NVIDIA OpenGL shader cache, `Separate Shader Objects`, the
  options and DLLs injected into the process. Cause unknown; the level is flat across matches.
- **Again 2026-10-13 (evening):** the OpenGL menu heap went from 19.1 to 22.5 MB between two runs a few hours apart. Not
  the game: the build before (cf97aeb) and after (dbf8452), alternated, both measured 22.5 MB, with and without VSync.
- **Expected values updated 2026-10-14** (CLAUDE.md, "Memory check"): after match 1 OpenGL is now about 26 MB (17.9 on
  2026-10-11, before the grass remake; part of it is the grass, flowers and reeds, part the drift above, not told apart), and the
  Direct3D 11 plateau is 90 to 97 MB (87 to 92 before), reached after 9 to 18 matches; the 100 MB plateau limit of
  `memcheck.ps1` left only 3 MB over the highest match of the last run, so it is 105 MB. Neither renderer grows per match.

## Tools and test method

- **DevTest switches** added along the way (CLAUDE.md lists them all): `-bench` (2026-10-04), `-strip` and `-bloomonly`
  (2026-10-10, for the bloom's flicker), `-nofocus` (2026-10-11: the window no longer takes Jonathan's keyboard focus;
  without it the game took the focus as soon as its window appeared), `-hide` (2026-10-11, to find which object draws
  something), `-nohud` (2026-10-11, clean review shots); `devtest.ps1 -GuiTour` (the GUI tour in a dev-size window).
- **`memcheck.ps1`** (2026-10-13, Jonathan: the 40-match runs recur after every step that adds GPU resources, so make the
  process a script and spend no tokens on reading 90 lines of numbers): both renderers in a row, judged against the
  expected values in CLAUDE.md, a PASS/FAIL block per renderer with the numbers, the comparison with the previous run
  and, on a FAIL, the evidence (what grew, which DLLs own it). Jonathan then asked for output an agent with no context
  can debug from, not just true/false. The analysis of a FAIL stays by hand. Its first test caught a false alarm: over 3
  matches one OpenGL step of 78 KB (OpenGL32 and the Windows text-input DLLs), so OpenGL growth is judged from 10
  matches on.
- **Smoke over the burnt grass** (2026-10-13; Jonathan picked it of three prototypes: cut grass, smoke, ember sparks).
  The prototype's smoke was a thin column in screenshots but a thick cloud in a `-stripdt` video of the same moments:
  Ogre's `ColourFader` loses at least 1/255 of alpha per frame (particle colours are bytes, truncated), so at the 800
  fps of uncapped test runs the -0.25/s fade ran at 3.1/s (docs/OGRE_BUGS.md, 16). Fixed with our `TumbuFader` for every
  template; at 144 fps the existing impacts barely change (their fades were close to the floor there), in uncapped test
  runs they now last as long as tuned. The smoke then glowed white at night (the smoke shader is unlit): tinted by the
  shared `sunColour`; a warm grey turned tan by day, so it is a cool grey now. Sparks from the embers were lost against
  the glowing embers (same orange, too small); cut grass was not wanted for now.
- **Burn slots and ground embers** (2026-10-14): with all eight burn slots in use, a ninth burn erased the oldest at
  once (a fully charred patch vanished in one frame). First fix: the oldest grows back over 3 s when the eighth
  arrives; but a ninth within those 3 s still dropped it part way (13 impacts 1.5 s apart: each drop about half burnt).
  Now 12 slots, two of them kept for fading (`GRASS_BURNS_SPARE`): the same 13 impacts drop one patch at 0.04 burnt.
  The shaders' loops stop at the live count, so the larger arrays cost nothing until more than 8 burns are live. The embers glowed only on the plants'
  tips, so a hit on bare earth or stone had none: small orange flecks now glow in the middle of the scorch for the
  first seconds (`tumbuGroundEmbers`). A first try (larger noise, `emberColour` x3) gave yellow puddles like lava. The
  stone's noise was 2D, a slanted projection of the world position, and stretched into thin streaks on the walls (seen
  with the field hidden); it is a 3D value noise now (less contrast, so the ember threshold went from 0.22 to 0.28).
- **Scorch on the stone** (2026-10-14): only the field drew the scorch, and a burn was a circle on the ground plan. A
  Genki Dama next to the ring scorched the ground up to the ring and left its edge clean; one landing on the seats
  (`groundBelow` found the podium top, within `burnHeight`) burnt the grass below and left smoke over clean stone.
  "Through the wall" was not real: the ground on the wall's far side is beyond the radius. Now a burn is a sphere
  around the impact's ground point (its height in `grassBurnY`) and the stone shaders scorch too, the bricks showing
  through the soot.
- **Frame-rate checks** (2026-10-13, Jonathan: test with VSync on, uncap only for benchmarks): DevTest logs every
  particle system's mean colour; the dust's mean alpha was 0 uncapped, 0.3 at 144 fps and 0.75 at 60 (Ogre's
  `ColourFader2` loses rises below one byte step a frame): the dust has been invisible in every uncapped test shot and
  about 2.5x fainter than designed in play. Now `TumbuFader` (0.8 at any rate) and `TumbuRandomiser` for its drift. The
  game code itself scales everything by the frame time (camera, animation, AI, effects; physics at a fixed 120 Hz).
  Test runs now wait for the monitor (`DevTest::useVSync`); `-bench`, `-fpscap`, `-novsync` run uncapped.
- **Test tools fixed** (2026-10-13, after the grass extras):
  - `-stripdt` / `devtest.ps1 -StripDt`: a strip's frames had been as far apart in game time as saving each took (the
    shockwave and the embers were over a few frames in); BaseApplication now runs Root::startRendering's loop itself
    and passes a fixed frame time while such a strip is saved.
  - `bench.ps1` runs a run with a hitch again (one 1 s window above 1.4x its median): one stalled second (2.95 ms
    against 1.55) had moved a `-Quick` result by 0.12 ms.
  - Helper scripts splat switches as a hashtable (an array splat passed `-HeroPos` and the rest positionally, twice).
  - `arena-flowers.ps1 -Only` and `-Preview`; and on a new Flowers.blend it no longer resets Blender's preferences
    (`read_factory_settings` switched the blender2ogre add-on off, and enabling it again failed halfway with a
    KeyError; the export only worked because the module stayed loaded): `read_homefile(use_empty=True)`.
- **Throwaway builds in `bin\scratch\`** (2026-10-10): Norton held every `TUMBU.exe` it found in a new place, so each run
  waited for its scan; `bin\` is excluded.
- **Dev runs at 1600x900 or smaller** (2026-10-10): a window as big as the screen took over Jonathan's work.
- **Benchmark method** (2026-10-11, tested with identical builds and a five-variant run):
  - 8 s runs measure the same as 15 s (5 s read a little low); within a run the 1 s windows spread by 4–5 %.
  - The first run of a session was always the fastest (a cold GPU), so each renderer starts with a thrown-away warm-up.
  - Direct3D 11 slows by several % over a session (759 → 704 fps in five rounds); with a fixed order the second variant
    looked ~0.02 ms slower, so rounds alternate (A B, B A) and the difference is taken round by round.
  - Identical builds come out 0.01 to 0.02 ms apart: the noise floor. Three rounds settle a clear difference.
  - Each run takes about 15 s, of which about 4 s is loading and quitting, so several variants in one launch would save
    little (and only for DevTest switches: 7 of the 34 variants in the history were switches, the rest builds).
  - The old "time outside rendering varied" warning was a false alarm: on OpenGL time moves between `render_ms` and
    `rest_ms` while their sum stays the same.
  - Over the history (fps-history.csv), OpenGL is as steady as Direct3D 11 in a typical benchmark (median spread 2.7 %
    against 2.8 % at 1920x1080), with a worse tail (the worst tenth spread 26 % against 11 %).
- **Frame-strip checks** (2026-10-11): torch flames (3.3 % crawl still, 3.2 % panning: their own flicker), dust (1.8 /
  2.0 %, smooth), SMAA on the robot outline (1.02 → 0.89 %), the ropes (0.18 %): nothing to fix.
- **PowerShell's `bash` is WSL** (2026-10-10): the variant switches of a bloom comparison never happened, so its runs
  were void.

## Incidents

- **GPU hang during a benchmark** (2026-10-11, 11:40): the NVIDIA driver timed out and Windows reset it (LiveKernelEvent
  141 and 1b8, watchdog dumps in `C:\Windows\LiveKernelReports`), with a corrected PCI Express error on the root port of
  the graphics card at the same second (WHEA 17). Every OpenGL launch then crashed while its renderer loaded, until the
  PC hung and was restarted (Kernel-Power 41). The game was running at ~700 fps then, with no long GPU work, so it points
  at the driver or the card's PCIe link, not the game. LiveKernelEvent 193 (driver live dumps) also appear on several
  days since 2026-09-15. If it happens again: look at these events first.
- **A failing hard disk on the development PC** (found 2026-10-11): Disk 0, a 2 TB Seagate ST2000DM001 with no
  partitions, reports a size of about 4 GB, and Windows logs that it retried a read at block 0 (event `disk` 153) every
  day: 1203 times between 2026-08-12 and 2026-10-11. Nothing of the project is on it (C:, D:, E: are other disks).
  Jonathan removed it the same evening (and cleaned the PC inside); after it: no disk retries, no WHEA error, every
  device OK, the graphics card at PCIe 4.0 x16 under load, and the bench at 1920x1080 as before (Direct3D 11 747 fps,
  OpenGL 622 fps, against 736 / 610 that morning). Then changed: XMP I in the BIOS (the memory runs at its rated 3200 MHz instead of 2133; booted cleanly, every bench run
  completed), both monitors at 144 Hz (they had been at 60; idle GPU power 21.5 W to 23 W, the memory clock still drops to
  idle), G-SYNC Compatible on the main monitor. At full uncapped load the card sits at 80-82 °C with its fans at about
  90 %, near its 83 °C target, which is within spec. The bench after XMP: 1920x1080 Direct3D 11 733 fps, OpenGL 607
  (GPU-bound, the same as before within the drift between sessions); 1024x768 Direct3D 11 1246 fps. Windows Memory
  Diagnostic (standard test, about 30 minutes) found no errors with XMP on (2026-10-12).
