# TUMBU – improvement ideas (lighting, shadows, environment)

Backlog collected after the lighting overhaul (phases 0–5: toon shading, shadows, glow/bloom, baked AO, god rays,
lens flare, contact shadows, fog, dust, SSAO). Each idea notes which reference game does it. Nothing here is
started. The special-attack visuals (DBZ/Naruto/Saint Seiya energy balls) are handled separately.

## Ideas, in order of impact

1. **Special attacks light the scene** *(Genshin, Astral Chain)*
   - **Done (2026-10) in the special-attack rework** (docs/SPECIAL_EFFECTS.md): four energy lights in the toon shaders.
   - Energy balls (Jyn, punch, kick) cast coloured light on the floor, walls and robots, and hits flash.
   - Today the toon shaders (`TumbuToon.h`) only use the sun, so the Jyn point light lights nothing.
   - A few point lights passed to the shaders as shared parameters (like `contactShadowA/B`) would do it.
   - Probably part of the special-attack rework.

2. **Robot "hero lighting"** *(Genshin)*
   - A soft fill/rim light that follows the camera, so robots always read well, even inside the coliseum's
     shadow.
   - Characters are lit separately from the scene; only the robot shader would change.

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

8. **Ki aura around the robot while Jyn charges** *(Dragon Ball Z, Saint Seiya cosmos)*
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

## Art side (not lighting, but the biggest visual gaps)

- **The coliseum has no texture** (flat colour), and the arena floor texture is near-white and dominates
  every shot.
- **The robot textures are plain.** A colour and material pass per robot would fit the anime style.
- Both are Blender work: `art/arena/Arena.blend` and `art/robots/` are the sources. Follow the Blender-first
  rule in CLAUDE.md.
