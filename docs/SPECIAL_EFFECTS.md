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
| Build-up | — | — | `jyn_charge`: first press (cast): the swarm balls appear around the robot and the eyes flare, nothing else; second press: the swarm balls fly in one after another and the ball, above the raised hand (its lower edge 0.25 above the knuckles), grows from nothing a step with each arrival (ready to throw once full); energy streaks from the air, motes rising from the floor, dust dragged to the feet, lightning over the ball (only once the ball has started to form); the swarm streams in (`jyn_mote` trails); `jyn_ball` light grows with the ball; eyes flare; from the second press a soft **ki aura** surrounds the robot (`jyn_aura`: glow shell, rising motes, a light), growing with the ball |
| Release | `punch_muzzle`: star flash at the fist | `kick_muzzle`: flash and sparks at the foot | `jyn_throw`: thick wake and big sparks; aimed at the nearest enemy within ~60 degrees in front, at `throwSpeed` (25 units/s, `skills.object`; 2011: ~33.5); the gathering winds down around the thrown ball (motes on their way chase it); the ki aura flares for ~0.15 s and fades over 0.5 s |
| Flight | orb + `punch_ball` (light, trail, sparks) | crescent (`Tumbu/EnergyCrescent`) + `kick_ball` | big orb + wake + light |
| Hit box | small box (the ball) | the crescent: 1.2 wide, 0.5 high | the ball (grows with it) |
| Impact | `blast_hit` on a robot (star flash, shockwave ring, sparks, light, small shake); `blast_wall` on the arena | same | `jyn_impact`: white frame, 70 ms hit-stop, toon explosion ball, ground shockwave, sparks, debris, smoke, light, shake; the special ends at the impact, so the robot lowers its arm right away |

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
- **Ki aura** (Saint Seiya cosmos with a soft DBZ touch; mock-up: https://claude.ai/artifact/QmnM2nMpBSPzDwqPMEZn6F,
  variant "Soft"): every `Part` builds a hidden twin entity (`Part::auraEntity`, the same mesh with
  `Tumbu/KiAura`) that shares the part's skeleton, so it follows every animation. `Robot::updateAura` (like the eye
  flare) shows it from the concentration to the throw: the level follows `SpecialJyn::getChargeGrowth`, flares at the
  throw and fades out, and is passed to the shaders as custom parameters (0: opacity, growth, width; 1: ki colour,
  rise speed). `robot_aura.vert/.frag` (`media/tumbu/robots/`) draw the inflated **back faces** only, like the
  outline, so the robot hides the shell except around its silhouette; fresnel makes it strongest next to the body,
  rising noise breaks it into tongues (in the shape and the bands), toon bands go near-white / ki / darker fringe, alpha
  blended so it reads over the white floor. Thinner and fainter around the head (the eye flare stays the focus).
  Colour: the Jyn ki colour (hero light blue, enemy crimson). The `aura` block of `effect jyn_aura` holds the shell's
  values (read by `Robot`, not an effect layer); the rest of that effect (rising motes `Tumbu/Fx/AuraMotes`, a light)
  is a normal held effect at the robot, released at the throw.
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
same as with `-nofx` within noise. With the ki aura (2026-10-03, three runs each, a full charge): about 540 fps against 560 with `-nofx`
(which also turns the aura off), within run-to-run noise (single runs ranged 360-810 fps). Budget per attack: under ~200 particles, at most four lights, no extra
full-screen passes (the flash is part of the final pass).

## Known issue: NVIDIA driver memory grows per match on Direct3D 11 (until a pool fills)

**It is bounded** (2026-10-09, `.\scripts\devtest.ps1 -Cycles 40`): the heap rises for about 18 matches, to about
85 MB, and then stays flat (84 to 88 MB over matches 18 to 40; private bytes about 535 MB). It is a driver pool that
fills up, not a leak, and a full 16-enemy game is a single match anyway. The per-match numbers below come from
12-match runs, which only see the pool filling; they also scatter from 1.5 to 5 MB per match between identical runs.

On Direct3D 11 the `-cycles` heap grows about 1.3 MB per match (1.7 to 2.3 MB when re-measured on 2026-10-04, with SMAA
on or off; 0.7 to 0.8 MB since the robots are skinned on the GPU, so part of it was the CPU-skinning upload path;
`memory-history.csv` keeps every measurement); on OpenGL it is flat, and every Ogre object count
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
- Also tried, also without effect: `IDXGIDevice3::Trim()` after each match (asks the driver to free its internal
  temporary memory, as Windows does when an app is suspended).
- **Proof that nothing leaks on our side:** with the Direct3D 11 debug layer, the live Direct3D objects reported by
  `ID3D11Debug::ReportLiveDeviceObjects` stay flat across 8 matches (647-655 objects; buffers 278-286, the per-frame
  ones; every other type exactly equal), while the heap grows ~1 MB per match. Neither Ogre nor the game keeps any
  Direct3D object; the memory is held inside the NVIDIA driver. The total private memory of the process is also
  roughly flat (441-449 MB from match 2), so the real cost may be smaller than the heap count suggests.

**Measured with** (re-check after updating any of these):

| | Version |
|---|---|
| GPU | NVIDIA GeForce RTX 3070 |
| NVIDIA driver | 617.42 (Windows driver version 32.0.16.1742); first measured with 617.14 (32.0.16.1714) |
| Ogre | 14.6.0 (Tsathoggua), Direct3D 11 render system |
| Windows | 11 Pro, build 26300 |
| Date | 2026-10-01; re-checked 2026-10-06 after a driver update (+1.13 MB per match on Direct3D 11, OpenGL flat) |

To re-check: `bin\Release\TUMBU.exe -cycles=8 -mute` on Direct3D 11, then compare the `heap=` of `memory cycle 2..8` in
`ogre.log` (about +1 MB per cycle with this setup; fixed if it stays within ~100 KB, as on OpenGL). The `heap growth
by block size` lines show whether the growing blocks still belong to `nvwgf2umx.dll`. The current versions are in
`ogre.log` (`Version 14.6.0`, `Driver Version:`).

**Reminder:** `-cycles` logs `[DEVTEST] REMINDER: ...` when the graphics driver or Ogre differs from the versions above
(the values are in `DevTest::logMemory`). Then re-check the issue, and whether the workaround is still needed: the
post-processing compositor is kept between matches (`Lighting::~Lighting` disables it instead of removing it), which
did not change the growth here. Update this section and the values in `DevTest::logMemory` afterwards.

**Live Direct3D objects:** needs Windows' optional "Graphics Tools" (admin: `dism /online /add-capability
/capabilityname:Tools.Graphics.DirectX~~~~0.0.1.0`). In the `[Direct3D11 Rendering Subsystem]` section of
`%USERPROFILE%\Tumbu\ogre.cfg` set `Debug Layer=On` and `Information Queue Exceptions Bottom Level=Corruption` (Ogre
only creates a debug device when that level is not "No information queue exceptions"); `-cycles` then logs
`[DEVTEST] d3d11 live objects: ...` per type after each match. Set both back afterwards: the debug layer is slow.
