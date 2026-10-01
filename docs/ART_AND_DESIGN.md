# TUMBU — Art direction and game design

Read this before creating or changing robots, parts, animations, specials or anything else about the game's
look or feel. The technical format rules are in the second half. For the rest of the project, see
`CLAUDE.md` and `docs/DEV_SETUP.md`.

## The vision

TUMBU is a **3D robot battle game in which you build your own robot.**

- **Customization is the core of the game, not the fighting.** A robot is assembled from 5 swappable parts
  (head, body, right arm, left arm, legs). The full game was meant to let the player **win or buy parts**
  and build their own robot. The current game is only a **proof of concept for the battle part**: 16
  enemies fought one after another in a single arena, and each defeated enemy drops one random part.
- **Robots with energy and spiritual powers.** Most mecha games use guns and blades. Here robots fight
  with **energy balls (ki)**, the way characters do in Dragon Ball Z, Saint Seiya and Megaman. That mix of
  mechanical bodies and spiritual energy attacks is the game's hook. **Every attack is an energy
  projectile**, including the "Punch" and "Kick" skills. New skills should keep this.

## Inspirations

### Robot design (modeling)
- **Medabots / Medarot** is the main influence and the source of the swappable-parts idea. Specific
  references Jonathan remembers:
  - the Medabots **skeleton** (the bare Medabot frame)
  - **Metabee**
  - **Rokusho**
- **Digimon**: **HolyAngemon**, plus other Digimon Jonathan no longer remembers specifically.
- A little **Gundam** and **Megaman**.

### Combat (fighting)
- **Megaman**, **Saint Seiya** and **Dragon Ball Z**: energy-ball / ki combat, charging up ("concentrating")
  before release, and bursts of many energy spheres.

Which inspiration went into which robot is **not recorded**. Don't claim that a given robot "is" Metabee or
anything else unless Jonathan confirms it.

## Visual style of the existing robots

Reference renders: `media/tumbu/robot00N/robot00N.jpg` (one per set), `media/images/robots.jpg` (a line-up),
and the portraits in `media/gui/imagesets/RobotFaces.png`.

**Common traits**, to keep for a consistent roster:
- A slim humanoid **Medabot-style frame**. Limbs show **thin exposed metal "bone" rods** (silver/chrome)
  between the armor pieces: forearms, shins, neck and waist.
- **Glowing round eyes, always two**, set in a dark visor slit (red, green or yellow).
- A **"T" emblem** (TUMBU) on the chest, and often on the shoulders or forearm shields.
- Glossy, specular, plastic-metal armor in 1–2 main colors plus black and silver trim, with engraved line
  details.
- Big, heavy boots or feet compared with the thin legs.

**Style progression.** Jonathan was learning 3D modeling while making these. **The early robots are
blocky and squared. The later ones are rounder and more sculpted.** New robots should follow the later,
rounder style unless asked otherwise.

| Set | Name | Tier (stats) | Look |
|---|---|---|---|
| `robot001` | **Lesser** | 1 (hp 20/part, atk 2) | Silver/white with navy accents, red eyes. Very boxy: octagonal head, inverted-triangle pelvis, block boots. The player's starting robot. |
| `robot002` | **Amber** | 2 (atk 3) | Red with black trim, green eyes. Pointed diamond head, triangular pelvis plate, angular boots. |
| `robot003` | **Buzzy** | 3 (hp 30, atk 5) | Purple with a silver face and tabard, red eyes. A single horn, spiked pauldrons, big cube forearms, flared boots. |
| `robot004` | **Guardian** | 4 (hp 30, atk 6) | Blue-violet with green accents, green eyes. Rounded head with a twin-spike crest, round shoulder shells, dark wing-like forearm guards, a plated skirt, fluted legs. |
| `robot005` | **Donn** | 5 (hp 50, atk 10) | Black and gold knight, yellow eyes. Round ridged helmet, gold pauldrons, big gold forearm shields, tall armored boots. The final boss set (`enemy17`). |

Enemy loadouts in `media/configuration/demo.object` climb through the tiers by mixing parts
(002 → 003 → 004 → 005), so **mixed-set robots must still look coherent**. Keep proportions and attach
points compatible across sets.

## Combat design

- Skills (`media/configuration/skills.object`):
  - **Punch** fires 1 energy ball.
  - **Kick** fires 1 energy ball.
  - **Jyn** is the special. It is a charge-up: `pre_special_jyn` concentrates, then `pos_special_jyn`
    releases a swarm of 10 energy balls.
  - Skills cost **AP** (energy, which regenerates) and gain XP and levels.
- **Guard** blocks. The `*_deflect` animations handle deflects.
- The Jyn swarm gathers by **homing** (default since 2026-10): each energy ball waits its turn, then flies into
  the Genki Dama (accelerating, turning a little, arcing up) and merges, so the ball grows one step per arrival.
  The 2011 **Particle Swarm Optimization (PSO)** of the college project is kept (`gather pso` in `skill jyn`,
  `skills.object`): each ball tracks its personal best and the global best position toward the target, weighted
  by inertia and the random factors `AC1`/`AC2`. It was replaced as the default because it sometimes left the
  ball away from the hand.
- Energy look (being reworked in 2026 towards anime ki attacks: DBZ, Naruto, Saint Seiya). **Jyn is Dragon Ball Z's
  Genki Dama (Spirit Bomb)**: energy gathered from all around (the swarm) into a ball above the raised hand (`SpecialJyn::getChargeAnchor`:
  the higher of the two hands' `finger_3_1` bones), growing a little with every swarm ball that arrives,
  blue-white (`kiColour` of `skill jyn` in `skills.object`). While it gathers, energy streaks in from the air and rises from
  the ground, faint dust is dragged over the floor and lightning crackles over the growing ball. Punch and kick are ki blasts in the robot's own colour:
  `kiColour` of its head's set (`robotNNN.object`), matching its eyes. Every ball is a procedural toon orb
  (`Tumbu/EnergyOrb`, `media/tumbu/effects/`): white-hot core, saturated ki-coloured ball with spiral streaks, a
  halo that blooms, and a dark ink ring so it reads on the white arena floor. Balls leave toon ribbon trails and shed
  sparks, and light the robots and the floor in their colour. The punch fires a ki ball from a star flash at the
  fist; the kick throws a crescent energy wave (`Tumbu/EnergyCrescent`) from a flash at the foot. Impacts: punch and kick blasts burst into a star flash,
  a shockwave ring and sparks; the Genki Dama explodes in a white frame and a short hit-stop, a banded toon
  explosion ball, a shockwave along the ground, debris and smoke, with a camera shake (`effects.object`).
- Robot stats are the **sum of the 5 parts** (hp, ap, attack, defense, velocity), so each part is a
  gameplay choice, not only a cosmetic one.

## Technical rules for robot assets (read before modeling)

A robot **set** is a folder `media/tumbu/robotNNN/`. The 3-digit suffix is required: `Part.cpp` builds
mesh names as `<part>_<last 3 chars of set name>.mesh`. A set contains:

| File | Purpose |
|---|---|
| `head_NNN`, `body_NNN`, `leftArm_NNN`, `rightArm_NNN`, `legs_NNN` `.mesh` + `.skeleton` | One mesh and one skeleton per part. The five share the same armature layout (it includes finger bones, for example `finger_4_2_L`). |
| `<part>UV_NNN.tga` | Diffuse map |
| `NM<part>UV_NNN.tga` | Normal map |
| `SM<part>UV_NNN.tga` | Specular map |
| `GM…` / `AO…` `.tga` | Glow and ambient-occlusion maps (AO is commented out in the materials) |
| `robotNNN.material` | Imports `robots.material` and inherits `base_material` (Cg shaders), setting texture aliases. Material names follow the 2.49 exporter's pattern: `<part>UV_NNN/TEXFACE/<part>UV_NNN.tga`. |
| `robotNNN.object` | `robotNNN set { name <DisplayName> }`, plus one block per part with `hp ap attack defense velocity` and `position x y z` (the attach offset on the robot) |
| `robotNNN.blend` | The source file. **Never overwrite the Blender 2.49 originals.** Save as a new file. |
| `robotNNN.jpg` | A reference render (dark blue backdrop, grey floor, 3/4 view) |

**Every part skeleton must contain all 14 animations, with these exact names** (see `Part.h` and
`media/configuration/animation.object`): `walk`, `right_punch`, `guard`, `no_pose`, `pre_special_jyn`,
`pos_special_jyn`, `left_punch`, `right_kick`, `run`, `dash_back`, `dash_left`, `dash_right`,
`left_up_deflect`, `right_up_deflect`.

**To add a new set `robot006`:**
1. Create the folder and files above.
2. Add `FileSystem=../../media/tumbu/robot006` to `[Game]` in `tumbu.cfg`, and to the copies in
   `bin/<Config>/`.
3. Add a 200×175 portrait named `robot006` to `RobotFaces.png` and `RobotFaces.imageset`. Conversations look
   up faces by set name, and the PNG will need to grow.
4. Use it in `demo.object` loadouts.
5. Export following the pipeline in `docs/DEV_SETUP.md` Part 3. Legacy Ogre 1.7 needs mesh v1.41, so
   convert with the old OgreXMLConverter.

Scale reference for `robot001`: the head sits at y≈1.73 and the legs at y≈0.54, so a robot is about 2 units
tall.
