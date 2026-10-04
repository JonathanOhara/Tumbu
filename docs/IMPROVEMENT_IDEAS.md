# TUMBU – next steps and improvement ideas

Backlog collected after the lighting overhaul (phases 0–5: toon shading, shadows, glow/bloom, baked AO, god rays,
lens flare, contact shadows, fog, dust, SSAO) and the special-attack rework of 2026-10 (docs/SPECIAL_EFFECTS.md).
Each idea notes which reference game does it.

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

3. **Metallic highlights for the robots** *(Genshin)*
   - A stylised matcap / sky-colour reflection on metal parts, so robots read as painted metal, not plastic.
   - Driven by the specular maps, or a new per-part metal mask.

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

## Art side (not lighting, but the biggest visual gaps)

- **The coliseum has no texture** (flat colour), and the arena floor texture is near-white and dominates
  every shot.
- **The robot textures are plain.** A colour and material pass per robot would fit the anime style.
- Both are Blender work: `art/arena/Arena.blend` and `art/robots/` are the sources. Follow the Blender-first
  rule in CLAUDE.md.

## Beyond the visuals

- **Windows installer**: `Tumbu.nsi` still has the 2011 x86 layout (DLL names, folders, vcredist). Update it for the
  x64 Ogre 14 build and its staged runtime (`cmake/StageRuntime.cmake`); the NSIS steps are in DEV_SETUP Part B.
- **Gameplay beyond the battle demo**: customization is the core of the game (ART_AND_DESIGN), but today parts are only
  won from defeated enemies. A part shop or loadout screen to build the robot between fights (buy parts, equip and
  compare them), then more arenas or a mode beyond the 16 fixed fights.
