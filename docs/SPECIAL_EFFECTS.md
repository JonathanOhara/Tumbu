# TUMBU — special-attack effects

How the energy attacks look and how the effect system is built. The art direction is in
[ART_AND_DESIGN.md](ART_AND_DESIGN.md); the rendering rules are in [CLAUDE.md](../CLAUDE.md).

## The look

Anime ki attacks (Dragon Ball Z, Naruto, Saint Seiya), drawn in the same toon style as the robots:

- **Hard tone bands, not smooth gradients:** a white-hot core, a saturated colour body, a soft glow. Only the core
  goes above the bloom threshold, so balls keep their colour instead of turning into white blobs.
- **A dark under-layer under every bright layer** (orbs, trails, rings, the crescent): an alpha-blended pass in a
  dark shade of the colour. Additive light alone disappears over the near-white arena floor and sunlit walls.
- **Timing:** a long build-up, a fast release, an impact that hangs (hit-stop and a white frame), then a slow fade.
- **Colours:** Jyn is always the Genki Dama blue-white (`kiColour` of `skill jyn`, `skills.object`). Punch and kick
  use the robot's ki colour: `kiColour` of its head's set (`robotNNN.object`), matching the eyes. Lesser red, Amber
  green, Buzzy crimson, Guardian jade, Donn gold. An enemy's Genki Dama uses `enemyKiColour` (crimson) instead, so it reads
  as hostile.

## The attacks

| Phase | Punch (ki blast) | Kick (crescent wave) | Jyn (Genki Dama) |
|---|---|---|---|
| Build-up | — | — | `jyn_charge`: first press (cast): the swarm balls appear around the robot and the eyes flare, nothing else; second press: the swarm balls fly in one after another and the ball, above the raised hand (its lower edge 0.25 above the knuckles), grows from nothing a step with each arrival (ready to throw once full); energy streaks from the air, motes rising from the floor, dust dragged to the feet, lightning over the ball (only once the ball has started to form); the swarm streams in (`jyn_mote` trails); `jyn_ball` light grows with the ball; eyes flare |
| Release | `punch_muzzle`: star flash at the fist | `kick_muzzle`: flash and sparks at the foot | `jyn_throw`: thick wake and big sparks; the gathering winds down around the thrown ball (motes on their way chase it) |
| Flight | orb + `punch_ball` (light, trail, sparks) | crescent (`Tumbu/EnergyCrescent`) + `kick_ball` | big orb + wake + light |
| Impact | `blast_hit` on a robot (star flash, shockwave ring, sparks, light, small shake); `blast_wall` on the arena | same | `jyn_impact`: white frame, 70 ms hit-stop, toon explosion ball, ground shockwave, sparks, debris, smoke, light, shake |

The Jyn swarm gathers by homing (`SpecialJyn::executaHoming`): every ball of the swarm is a visible orb with its own
stream, flies in on its turn and merges; the swarm, the ball and the gathering follow the robot while it walks. The 2011
Particle Swarm Optimization is kept as an option (`gather pso` in `skill jyn`).

## How it is built

- **`EffectsManager`** (owned by `Demo`, one per match) runs the effects, so an impact outlives its projectile.
  `spawn(name, position, kiColour, held)`: a one-shot effect deletes itself when done; a held one is moved by its
  owner (`setPosition`, `setIntensity` 0..1, `setScale` = the ball's radius) and let go with `release()`.
  Specials keep their held effects in `SpecialInterface::ballEffect` (and `SpecialJyn` its charge, throw and stream
  effects) and release them in `clear()`.
- **Effects are data:** `media/configuration/effects.object`, one `effect <name>` with a block per layer. The keys are
  documented at the top of the file. Layers (`EffectLayers.cpp`):
  - `light`: a coloured point light for the toon shaders (see below)
  - `screen`: screen flash, camera shake, hit-stop
  - `trail`: an Ogre `RibbonTrail` following the effect
  - `particles`: an Ogre particle system from a template in `media/tumbu/effects/effects.particle`
  - `converge`: a pool of motes moved in C++ towards the effect (the Genki Dama gathering)
  - `lightning`: a `BillboardChain` of jagged arcs over a sphere, rebuilt every ~70 ms
- **Shaders** (`media/tumbu/effects/`, unified GLSL/HLSL, all procedural, no textures): `energy_orb` (balls),
  `energy_trail` (trails and lightning), `energy_spark`, `energy_crescent`, `energy_burst` (FLASH, RING, DOME, SMOKE,
  DUST, DEBRIS). The particle colour carries the ki colour; its alpha is a random seed (orbs) or the life left
  (impact pieces, driven by a ColourFader), so the shaders animate without per-particle uniforms.
- **Scene lights:** up to four energy lights reach the robot and arena shaders as shared parameters
  (`energyLightPos0..3`, `energyLightColour0..3`; `tumbuEnergyLights` in `TumbuToon.h`, a toon band with a
  quadratic falloff). The strongest four requests of each frame win. The terrain and particles do not receive them.
- **Screen:** `screenFlash` (shared parameter, mixed in at the end of the final post pass), a camera shake node
  between the zoom node and the camera, and a hit-stop that slows robots, AI and physics
  (`EffectsManager::gameTime`) and particles (controller time factor).

## Testing

- `devtest.ps1 -FxTest <effect> [-FxTime s] [-FxDistance d]`: plays an effect once in front of the hero and takes
  the screenshot `s` seconds after it starts; `move:<effect>` holds it and circles it (trails, sparks).
- `-FxTest special:punch|kick|jyn|jynthrow`: the hero uses the attack (`jynthrow` charges, throws, and shoots
  `FxTime` after the throw).
- `-JynWalk`: charges Jyn while walking and logs the ball's offset from where it gathers.
- `-JynHit [D]`: the enemy stands still D units (6) in front of the hero (no AI) and the hero throws a Genki Dama at
  it: a real hit (the impact on a robot), shot `FxTime` after the throw. `TUMBU.exe -jynhit=enemy` reverses it: the
  enemy charges and throws at the hero.
- `TUMBU.exe -nofx`: no effects at all, to compare frame rate and memory.

## Performance

Measured on the dev PC (Direct3D 11, 1024x768): about 650 fps while a Genki Dama charges with every layer on, the
same as with `-nofx` within noise. Budget per attack: under ~200 particles, at most four lights, no extra
full-screen passes (the flash is part of the final pass).

## Known issue: NVIDIA driver memory grows per match on Direct3D 11

On Direct3D 11 the `-cycles` heap grows about 1.3 MB per match; on OpenGL it is flat, and every Ogre object count
returns to the same value each match, so the game frees everything it creates. Investigated in 2026-10 with the heap
histogram of `-cycles` (`heap growth by block size`, plus the DLL that owns the pointers in a sample block):

- The growing blocks (5944, 1680, 1552, 1048 and 2x1560 bytes, 40-100 sets per match) belong to the NVIDIA user-mode
  driver (`nvwgf2umx.dll`).
- **Not the effects:** spawning an effect every 0.5 s through whole matches with every other effect off adds nothing
  measurable; pooling trails, chains and particle systems, and dynamic index buffers for the chains, changed nothing
  (all reverted).
- **Tied to post-processing:** without the compositor the growth drops to about 0.5 MB per match (twice, each way).
  Keeping the compositor between matches (disabled, not removed: `Lighting::~Lighting`) and skipping the per-frame
  shadow-map bind of the god-ray pass did not change it. The cause is inside the driver.
- It only happens when you go back to the menu and start another match: a normal game (16 enemies in one match)
  never triggers it.
- Also tried without effect: forcing the driver to release deferred objects after each match
  (`ID3D11DeviceContext::ClearState` + `Flush`), shadows off, and the simple skydome instead of Caelum (about 1 MB per
  match in every case). Not worth more time: about 1 MB per return to the menu, and the driver frees it when the game
  closes.
