# TUMBU – next steps and improvement ideas

The open backlog. Each idea notes which reference game does it. **Finished work moves to
[docs/HISTORY.md](HISTORY.md)** (with its decisions and measurements); here a done item keeps one line and its open
follow-ups.

## Priority order of the open items (2026-10-04, updated 2026-10-11)

Visual track, most important first (impact on what the player sees, against the effort). Done: SMAA anti-aliasing,
the arena remake, the night arena lights, the painted toon sky, the mip-chain bloom (docs/HISTORY.md).

1. **Tune Jyn in real fights** (idea 9): no code, only play time.
2. **Grass remake** (Art side) — done (2026-10-12): the field from Blender, the meadow shader and the toon tall grass;
   extras open (see "Grass remake").
3. **Robot art pass:** richer textures and toon-friendly normals (Art side).
4. **Cloud shadows** (idea 5).
5. **Colour-grading tables per time of day** (idea 6).
6. **Linear workflow** (Rendering modernization 4): only with a full retune of the look. Revisit Khronos PBR Neutral tone
   mapping then (Rendering modernization 5).
7. **GTAO** (Rendering modernization 6) and the **smaller extras** (idea 7).
8. **OpenGL frame rate** (Rendering modernization 9): about 30 % behind Direct3D 11; only worth it if OpenGL becomes a
   target (a Linux build).

Separate tracks: **gameplay beyond the battle demo** (part shop or loadout screen) is the most important item for the game
itself (customization is its core), but it is design and code, not visuals; the **Windows installer** comes last, once
the improvements are done (Jonathan, 2026-10-04).

## Ideas, in order of impact

1. **Special attacks light the scene** — done (2026-10).
2. **Robot "hero lighting"** — done (2026-10). Possible follow-ups: black parts stay black (the fill multiplies the
   albedo), so they read only through the rim and outline; a Genshin-style screen-space depth rim, or softer cast shadows
   on robots, would go further.
3. **Metallic highlights for the robots** — done (2026-10). Possible follow-ups: painted metal masks in Blender if
   specific spots shine wrongly (engraved lines, emblems); the streak axis is world-vertical, so a raised arm gets it
   across rather than along.
4. **Night arena lights** — done (2026-10-09). Possible follow-ups: the grass gets the torches' light with the grass
   remake (its own shader); the torches could light the dust; lanterns in the windows as emissive glow only (no light,
   no cost); a slow pulse on the neon.
5. **Cloud shadows** *(Breath of the Wild)*
   - A scrolling noise texture multiplied into the sun term (and the god rays), drifting with the wind.
   - Cheap, and makes the scene feel alive.
6. **Colour-grading look-up tables per time of day** *(BotW, Genshin)*
   - A 3D look-up table in the final pass, blended between keyframes in `lighting.object`.
   - Gives richer golden-hour and night moods than exposure and tint alone.
7. **Smaller extras**
   - A subtle floor reflection: planar or screen-space, Astral Chain style.
   - Chromatic aberration on hits (the screen flash and shake are done).
   - Grass and foliage sway in the wind (BotW).
   - Per-robot light direction and crisper two-tone shading options (Granblue Fantasy Versus).
   - Dust spawned only in sunlit air, so the beams carry more of it.

## Special-attack ideas (to test)

8. **Ki aura while Jyn charges** — done (2026-10, docs/SPECIAL_EFFECTS.md).
9. **Tune Jyn in real fights** (no code: `skill jyn` in `media/configuration/skills.object`)
   - Does `throwSpeed 25` leave enough time to guard or dodge? Is the auto-aim (nearest enemy within ~60 degrees in
     front) fair? Does the enemy's crimson Genki Dama (`enemyKiColour`) read well? Is the ~2 s gathering too slow?
   - Scripted tests (`devtest.ps1 -JynHit`, `-FxTest`) only check that it works, not how it feels.

## Rendering modernization (review of 2026-10-04)

Techniques in use that have a clear modern replacement, most valuable first:

1. **Anti-aliasing** — done (2026-10): SMAA 1x. Open follow-ups, small: a quality choice (Low / High) in Options; colour
   edge detection if edges that differ only in hue (a red part against a green one) stay jagged.
2. **Bloom** — done (2026-10-11): the mip chain. **Possible later, low priority**: it costs about 0.10 ms (Direct3D 11) /
   0.14 ms (OpenGL) at 1920x1080, half of it the fixed cost of its 11 passes; none of these is worth it at 1.4 % of a
   144 fps frame:
   - a **compute-shader downsample** (one dispatch for the six downsamples, like AMD FidelityFX SPD): saves about 5 pass
     costs, ~0.02 to 0.03 ms. It must write our separate level textures (docs/OGRE_BUGS.md, 14); OpenGL binds images
     differently; the 13-tap filter needs neighbours across thread-group tiles. The five upsamples stay passes.
   - **fold the last upsample into the final pass** (one half-size pass less, ~0.01 ms; a tent there cost 0.04 ms at full
     size, so it needs care);
   - a **Bloom quality option** (6 / 3 levels / off) for weak GPUs, where the per-pixel part grows: 3 levels save
     0.015 / 0.04 ms here, off saves 0.10 / 0.14 ms.
3. **Sky** — done (2026-10-06): our own painted toon sky replaced Caelum.
4. **Lighting in gamma space** (`lighting.object` colours are display space; no sRGB conversion). The modern standard is
   a **linear workflow** (sRGB textures decoded, light added in linear, encoded at the end): energy lights, bloom and fog
   blend correctly. Big retune of every value, so only together with a larger look pass; many toon games accept
   gamma-space lighting. When this is done, try **Khronos PBR Neutral** tone mapping again (item 5).
5. **Tone mapping: a per-channel soft shoulder.** Bright saturated colours shift hue as their channels roll off at
   different rates (an orange highlight turns yellow). **Khronos PBR Neutral** (2024) keeps base colours 1:1 up to a
   point and compresses only highlights, hue-preserving. Tried 2026-10-10 and kept out on our display-space image
   (docs/HISTORY.md); **revisit it together with the linear workflow (item 4)**: PBR Neutral expects linear light.
6. **SSAO: 12 spiral samples.** **GTAO** (ground-truth AO) gives more accurate contact darkening for a similar cost.
   Low priority: the arena and robots also have baked AO.
7. **Uncompressed TGA textures on the robots** (the arena is BC7/BC5/BC4 DDS since 2026-10-06): BC7 colour and BC5
   normal maps would take about 4x less video memory and load faster, no visible change. Do it with the robot art pass.
8. **Robots skinned on the CPU** — done (2026-10): skinned on the GPU.
9. **OpenGL about 30 % slower than Direct3D 11** (not investigated yet; noted 2026-10-06).
   - Measured with `bench.ps1` on the same scene and window size (RTX 3070, Ogre 14.6): tiers view before the arena remake
     1443 vs 1008 fps at 1024x768 (-30 %), 1127 vs 837 at 1920x1080 (-26 %); robot fight after GPU skinning 1371 vs 937.
     Every arena-remake step changed both renderers by about the same amount, so the gap is older than it.
   - Where the time goes: frames are about 1 ms, mostly the CPU submitting draw calls. The bench's `render_ms` is about
     1.18 ms on OpenGL against 0.91 ms on Direct3D 11 for the same scene: Ogre's GL3+ render system costs more per draw
     (uniforms uploaded per pass and hashed for its cache, more state changes) than its D3D11 one (constant buffers).
   - Things to try, measured with `bench.ps1 -Renderers GL`: count the draw calls and passes per frame (Ogre's frame
     stats, RenderDoc / Nsight); fewer draws for the static arena (one entity per material, static geometry); shared
     parameters through uniform buffers if Ogre's GL3+ supports it for our programs; compare with Ogre's own samples to
     see whether the gap is ours or the render system's.
   - Priority: low. With VSync or the 144 fps cap both renderers run far above the limit, and Direct3D 11 is the default.

Still modern, keep: inverted-hull outlines (Genshin, Guilty Gear), the shadow-map raymarched god rays, integrated depth
shadows with normal offset (a single map is right for an arena this small), the fixed-step physics.

## Art side (not lighting, but the biggest visual gaps)

- **Arena remake** (coliseum retexture with parallax occlusion and self-shadows, the stone-tile ring, the red neon, the
  painted sky) — done (2026-10-06, docs/HISTORY.md). Note: the parallax suits the stone, not flat surfaces; the sun's
  shadow map and the contact shadows still land on the flat surface, only the stones' own shadows follow the depth.
- **Grass remake** (the field between the ring and the walls; decisions in docs/HISTORY.md, "Grass remake"):
  - The field as a mesh from Blender with the wild-meadow toon shader, and the toon tall grass (generated at load from
    the field map, bent by the wind, the robots' feet and low energy balls) — done (2026-10-12).
  - If the cost matters later (+0.15 to +0.24 ms at 1920x1080): thin the far chunks (fewer blades beyond 20 units, wider
    ones), as Ghost of Tsushima does.
  - Extras: flowers, reeds, the impacts' shockwave across the field and burnt grass where a Genki Dama lands — done
    (2026-10-13; CLAUDE.md, "The tall grass"), and smoke over a fresh burn. Prototyped, not taken for now: cut grass
    where a punch or kick ball hits near the ground (short stubs with pale cut ends, growing back), sparks leaving the
    embers (they need to be brighter and yellower than the embers to show).
  - **Flowers** (Jonathan, 2026-10-13, after Xenoblade 3's meadows): in the games that document it, flowers are not
    made by the blade generator but are small artist-made models drawn many at once and placed by the same procedural
    rules as the grass (Ghost of Tsushima: GPU-instanced flowers and pampas grass placed in its grass tiles; Horizon Zero
    Dawn: almost all vegetation placed at run time from density maps). Xenoblade 3 has no public talk; its screenshots
    show the same split (blades, plus clumps of stems with flower heads). For us: a few flower models made in Blender
    (stem, leaves, a toon head), copied into the grass chunks by `Grass` where a field-map channel or a drift noise says
    so, bent by the same wind and push (root and height in uv1). First look done (2026-10-13): five kinds, ten clumps
    around the ring (CLAUDE.md, "The flowers"); next, after Jonathan's look: how many, where (drifts, or a field-map
    channel), more kinds.
  - Small things seen on the way (2026-10-13):
    - Two of the ten flower clumps found no ground (all their tries fell on the earth or under the walls): a clump
      could try another centre.
    - Where the flowers grow could be painted: the field map's alpha is free (`field_map.png` uses R, G, B). Or the
      clumps could follow the flower dots painted in `meadow_grass_col`, so the 3D flowers and the painted ones agree.
    - At night the blue forget-me-nots come out very saturated under the blue moonlight (the white daisies turn blue
      like the snow); a per-flower cap on the night tint if it bothers.
    - The flowers' normals do not follow their sway (the light ignores the bend); invisible so far.
    - The snow's drift texture is projected from above, so on the steepest faces it stretches into streaks; a
      triplanar lookup on steep faces would keep the drifts round.
    - The scorch is drawn by the field shader only: a Genki Dama that lands just outside the ring scorches the earth
      band and the grass up to the ring, while the ring floor inside the same radius stays clean. Small; the ring
      floor's shader (`env_stone_floor_ps`) could take the same `TUMBU_GRASS_BURNT` darkening.
- **The robot textures are plain.** A colour and material pass per robot would fit the anime style.
- Both are Blender work: `art/arena/Arena.blend` and `art/robots/` are the sources. Follow the Blender-first rule in
  CLAUDE.md.
- **Toon-friendly normals for the robots** *(Guilty Gear Xrd, Genshin)*: the modern anime-game answer to "better normal
  mapping" is not more surface detail but **cleaner shading**. Edit the robots' vertex normals in Blender (Data Transfer
  from a smooth proxy shape, Normal Edit / Weighted Normal modifiers), so the toon bands fall in clean, deliberate shapes
  instead of following every polygon; then bake the 2011 normal maps again from a high-poly version (bevels, panel lines)
  for the rounder, later style. Every lighting feature gains from it (sun bands, hero fill, metal reflection, streak). An
  art pass per robot; needs the 14 animations kept (DEV_SETUP Part B).
- **Not recommended for the robots: parallax mapping and tessellation.** Both suit detailed realistic surfaces (stone,
  bricks, terrain), but the robots are flat-shaded toon armour with no height maps, and on a slim, moving silhouette the
  effect would barely show. Extra detail simply goes into the mesh.

## Beyond the visuals

- **Windows installer**: `Tumbu.nsi` still has the 2011 x86 layout (DLL names, folders, vcredist). Update it for the
  x64 Ogre 14 build and its staged runtime (`cmake/StageRuntime.cmake`); the NSIS steps are in DEV_SETUP Part B.
- **Gameplay beyond the battle demo**: customization is the core of the game (ART_AND_DESIGN), but today parts are only
  won from defeated enemies. A part shop or loadout screen to build the robot between fights (buy parts, equip and
  compare them), then more arenas or a mode beyond the 16 fixed fights.
- **Report our Ogre bugs upstream** (docs/OGRE_BUGS.md): the three patched ones as issues or pull requests to
  OGRECave/ogre, so the patches can go. Jonathan's call (it posts publicly); not started.
- **One shared robot skeleton (a rig file as the source)**: found while building the robot export (2026-10-05).
  - Today: each `art/robots/robot00N.blend` has its own armature. All five share the same 50-bone hierarchy, bone names
    and 14 actions, but the proportions differ by up to 8 cm and a few action tracks differ. The game gets 25 skeleton
    files, one per part, each a copy of its robot's armature moved to the part's origin.
  - Idea: `art/robots/rig.blend` holds only the armature and the 14 actions, and is the source for the skeleton and the
    animations. Robot files either **link** it (Blender library linking with a library override, so a robot can adjust
    its proportions while the actions stay shared) or **append** it once as the starting point for a new robot.
  - In Ogre:
    - **One skeleton per robot** instead of one per part. The parts share one skeleton instance
      (`Entity::shareSkeletonInstanceWith`, already used for the ki aura shell), so each robot updates its animation
      once instead of five times.
    - **Optionally one animation set for every robot:** `Skeleton::addLinkedSkeletonAnimationSource` plays the
      animations of a master skeleton on any skeleton with the same bone names (bone offsets scaled).
  - What it touches: parts are swapped between robots, so robots with different proportions need attach points that
    still line up; the per-part re-centring and `robot00N.object` would change, along with `Part` / `Robot`; the export
    (`scripts/export-robots.ps1`) would write one skeleton per robot.
  - Gain: animate once; new robots start from the rig; mixed robots move consistently; fewer files.
