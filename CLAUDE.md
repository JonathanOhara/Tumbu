# TUMBU — project guide for Claude

TUMBU is a 3D robot arena-fighting game written in C++ on **Ogre3D 1.7**. It started as Jonathan's college
final project (TCC, 2010–2011). Authors: Jonathan Ohara de Araujo and Luiz Fernando Dubas ("ShyDS Games").
Jonathan made all code, models, textures, music and UI. The last real development happened in Aug 2011; the
last build was produced in Oct 2012. The goal now is to **revive** it: first run it as-is, then modernize.

Step-by-step setup (run the old build, rebuild with the original toolchain, modernization path):
**[docs/DEV_SETUP.md](docs/DEV_SETUP.md)**.

**Before any work on robots, models, parts, animations, specials or the game's look and feel, read
[docs/ART_AND_DESIGN.md](docs/ART_AND_DESIGN.md).** It covers the vision (robot customization is the core,
with energy/ki combat), the inspirations (Medabots, Digimon, Gundam/Megaman, DBZ and Saint Seiya), the style
of each robot, and the asset format rules.

The game in one line: build a robot from 5 swappable parts, then fight other robots with energy-ball (ki)
attacks. The current game is a battle-only proof of concept: 16 enemies in one arena, and you win a part
from each one you defeat.

## Build and run (legacy toolchain, working since 2026-09-28)

```powershell
.\scripts\build.ps1            # VC++ 2010 compiler via VS2022 MSBuild → bin\Release\TUMBU.exe  (-Configuration Debug, -Rebuild)
.\scripts\run.ps1              # copies runtime DLLs + cfgs, ensures %USERPROFILE%\Tumbu, launches the game
```

- To check a run, read `%USERPROFILE%\Tumbu\ogre.log` (and `cegui.log`). Harmless noise: `white.png` and
  `city_6_*.dds` not found.
- The toolchain on this machine: VS2022 Community (IDE + MSBuild), plus Windows SDK 7.1 + the VC++ 2010 SP1
  compiler (`PlatformToolset=Windows7.1SDK`, compiler 16.00.40219.01). There is also the Ogre 1.7.3 VC10 SDK
  at `D:\OgreSDK` with addons in `D:\OgreSDK\addons` (restored from backup), and the OpenAL 1.1 SDK.
  Dependency paths come from user environment variables. The setup steps and quirks (the `Common7\IDE`
  PATH entry and the `ammintrin.h` stand-in) are in `docs/DEV_SETUP.md` Part 2.
- **Do not change the toolset to v140 or newer.** Every prebuilt dependency is VC++ 2010 (MSVCR100), and
  the C++ ABI differs. A newer compiler means the full modernization (DEV_SETUP Part 4).
- Runtime DLLs are taken from the 2011 build folder (below). They are not in the repo: `.gitignore`
  excludes `*.dll`, `*.exe` and `*.lib`.

## Current state

- The repo was imported from SVN into git in one commit (`906697a`). There is no CMake, only MSVC project files.
- **The original 2011 build** is at `E:\WorkSpaces\workspace tcc\Tumbu\bin\Release\TUMBU.exe` (Dec 2011). It is
  the old SVN working copy, and its `src/` and `include/` were byte-identical to this repo at import. It still
  runs, which makes it a useful reference for "how it behaved before".
- **The original SDK and addons are backed up** at `D:\Backup\OgreSDK\` (4.8 GB). Most addons were built with
  VC++ 2010 on 2011-07-24. The Ogre SDK in that backup is 1.8.1 and unused; the game uses Ogre 1.7.3.
- Blender 5.2 is installed. `media/tumbu/arena/coliseum.blend` was re-saved by **Blender 4.0.1** (Jul 2024)
  and is uncommitted; its file header reads `BLENDER-v401`. Every other `.blend` is still Blender **2.49**.
- **Known gameplay bug: movement depends on frame rate.** `Character::updateMovement` sets the velocity to
  `RUN_SPEED * frameTime`, so the hero crawls at high FPS. `CharacterEnemy::updateMovement` uses the full
  `RUN_SPEED` and teleports its rigid body to the node every frame, so the enemy can fly out of the arena.
  Both were tuned for about 60 FPS in 2011. Both are visible on the current PC (RTX 3070).
- The game design used Portuguese. Some identifiers are Portuguese (`sofrerDano` = take damage,
  `criarDano` = deal damage), and so are many comments and `printf`s.

## Tech stack (what the code links against)

| Area | Library (version used) | Where in code | Status today |
|---|---|---|---|
| Rendering | Ogre 1.7.x "Cthugha": 1.7.2 during development (early 2011), final DLLs dated Jun 2011 (possibly 1.7.3). D3D9 + GL render systems | everywhere | Ogre 1.x is still maintained (14.x). Needs API porting. |
| Terrain | OgreTerrain + OgrePaging (1.7) | `DotSceneLoader`, `Demo::createTerrainPhysic` | Still part of Ogre 14 |
| Debug UI | OgreBites `SdkTrays` / `SdkCameraMan` (from the 1.7 samples) | `BaseApplication` | Still in Ogre 14 (OgreBites) |
| Input | OIS 1.3 (keyboard, mouse, joystick) | `BaseApplication`, every `*Listener` | OIS is still on GitHub, or use SDL2 via OgreBites |
| GUI | CEGUI **0.7.5** + OgreRenderer, TaharezLook skin | `GUI`, `Dialog`/`Alert`/`Confirm`/`Conversation`/`ShowPart`, `Log*`, `SkillHit*` | 0.8.7 is the last stable. Big API change from 0.7. |
| Physics | Bullet **2.77** + **OgreBullet** (ogreaddons SVN r2979) | `SimpleRigidBody`, `Robot`/`Character*`, `Special*`, `Demo` | OgreBullet is dead. Ogre 13+ has its own `Bullet` component. |
| Audio | **OgreAL** (Eihort) + OpenAL (Creative 1.1) + Ogg/Vorbis | `Sound`, `SoundManager` | OgreAL is dead. Replace with OpenAL Soft or miniaudio. |
| Sky | **SkyX 0.2** (only when "sky quality" = 1, Windows only through `#ifdef _WINDOWS`) | `Sky` | Dead. Low-quality mode uses a plain skydome material. |
| Vegetation | **PagedGeometry** (static lib) | `DotSceneLoader::processPagedGeometry` | Maintained fork: OGRECave/ogre-pagedgeometry |
| Shaders | NVIDIA **Cg** (`general.cg` through `Plugin_CgProgramManager`, `cg.dll`) | `media/tumbu/robots/*` | Cg was abandoned in 2012. Port to HLSL/GLSL or RTSS. |
| Scene format | `.scene` from **Ogitor 0.4.4** plus a terrain page `.ogt`, parsed by `DotSceneLoader` (rapidxml) | `media/scenes/arena` | The loader is bundled in `src/` |
| Leak check (Debug) | Visual Leak Detector (`vld.h`, hardcoded `D:\OgreSDK\addons\vld`) | `TUMBU.cpp` | Optional |
| Compiler | **Visual C++ 2010 Express** (x86 only, MSVCR100), VS2008 project also present; Code::Blocks on Linux | `TUMBU.vcxproj` / `TUMBU.vcproj` | See DEV_SETUP |
| Installer | **NSIS** script `Tumbu.nsi` ("Tumbu Beta 1.50") | root | The compiled installer was not found on any disk |

## Build configuration facts

- Solution `TUMBU.sln` (VC++ Express 2010) is the one in use. `TUMBU vs2008.sln` and `TUMBU.vcproj` are legacy.
- Win32 only. Output goes to `bin\$(Configuration)\TUMBU.exe` and intermediates to `obj\$(Configuration)\`.
- Debug uses `/SUBSYSTEM:CONSOLE` with entry point `WinMainCRTStartup`, so a console shows `printf`/`cout`.
  Release uses the Windows subsystem.
- The project reads these **environment variables**: `OGRE_HOME`, `OGRE_BULLET_HOME`, `BULLET_HOME`,
  `OGREAL_SDK`, `OPENAL_SDK`, `LIB_OGG_HOME`, `LIB_VORBIS_HOME`, `CEGUI_HOME`, `SKYX_HOME`,
  `PAGED_GEOMETRY_HOME`. It also expects `$(OGRE_HOME)\boost_1_44` and `$(OGRE_HOME)\addons\skyx\...`.
  The mapping to the `D:\Backup\OgreSDK` folders is in DEV_SETUP.
- Link libraries: OgreMain, OIS, OgrePaging, OgreTerrain, CEGUIBase, CEGUIOgreRenderer, SkyX,
  OgreBulletCollisions/Dynamics, bulletcollision, bulletdynamics, LinearMath, GIMPACTutils,
  ConvexDecomposition, OpenAL32, OgreAL and PagedGeometry. Debug uses `_d` suffixes for the Ogre, OIS, CEGUI,
  SkyX and OgreBullet libs.
- `PRECOMP` is defined, but precompiled headers are off.

## Runtime layout (important when running)

- The working directory must be `bin\Release` (or `bin\Debug`). Resource paths in `tumbu.cfg` are relative
  (`../../media/...`).
- Files next to the exe: `plugins.cfg` / `plugins_d.cfg` (Ogre plugins), `tumbu.cfg` / `tumbu_d.cfg` (the
  resource groups, a copy of the root `tumbu.cfg`), and all the DLLs.
- **Per-user folder `%USERPROFILE%\Tumbu\`** holds `ogre.cfg` (video settings), `ogre.log` and `cegui.log`
  (`BaseApplication::setup`, `GUI` constructor). **It must exist.** The NSIS installer creates it. Without
  it, Ogre cannot save `ogre.cfg` after the config dialog and throws.
- Resource groups initialised at startup: Essential, General, imagesets, Fonts, Schemes, LookNFeel, Layouts.
  When a match starts, `Demo::initialiseGameResources` loads EditorResources, Brushes, Plants,
  TerrainTextures and Game, plus SkyX if sky quality is 1. `media/EditorResources`, `media/brushes` and
  `media/tumbu/plants` are listed in `tumbu.cfg` but do not exist. Ogre 1.7 tolerated that.
- Controls: WASD to move, 1/U punch, 2/I kick, 3/O special "Jyn", Left Ctrl/P guard, Q/E rotate the camera,
  the mouse controls the camera and cursor, ESC opens the menu or cancels, Space confirms. Joysticks are
  supported. Debug-only keys: G shows the camera panel, R toggles wireframe, F5 reloads textures, SysRq takes
  a screenshot, and F12 prints the scene graph.

## Code architecture (`src/` + `include/`, about 13k lines of own code)

Everything is a set of **singletons plus Ogre `FrameListener`s** registered on `Ogre::Root`. Input fans out
through listener maps in `BaseApplication`.

- `Main.cpp` has `WinMain`. It calls `TUMBU::getInstance()->go()` and shows Ogre exceptions in a MessageBox.
- `BaseApplication`: an Ogre tutorial-framework base. It creates Root, config, resources, camera and
  viewport, sets up OIS, SdkTrays and the debug panel, and keeps maps of Key/Mouse/Joystick/Collision
  listeners (`addKeyListener(obj, "name")` and so on).
- `TUMBU` (singleton, extends BaseApplication) is the game root. It holds the game state
  (`TumbuEnums::GameState`: NONE, START_SCREEN, IN_DIALOG, PAUSED, LOADING, PLAYING) and the options (shadows,
  sky quality). It owns the StartScreen, AIManager, SoundManager, Clock and GUI. `initializeDemo()` and
  `finishDemo()` switch between the menu and a match. `createSimpleRigidBody()` builds trimesh statics.
- `StartScreen` / `CutScene` is the title-screen background.
- `Demo` runs one match. It loads `Arena.scene` through `DotSceneLoader`, then sets up lights and shadows, the
  OgreBullet `DynamicsWorld`, the Sky, the terrain heightfield physics, the arena and coliseum trimesh
  physics, the hero (`Character`), the enemy (`CharacterEnemy`) and the `Camera`. Each frame it steps physics,
  dispatches collisions manually (manifolds are mapped to `CollisionDetectionListener`s by rigid-body name)
  and checks for deaths. When an enemy dies, the hero wins a random part from it and the next enemy spawns.
  The match ends after `demo configuration { enemies N }`.
- `Robot` (abstract) → `Character` (player, reads input) and `CharacterEnemy` (AI-driven). A robot is **5
  swappable `Part`s**: HEAD, BODY, RIGHT_ARM, LEFT_ARM, LEGS. Each part has its own mesh and skeleton and adds
  hp/ap/attack/defense/velocity. There are 14 animations (`ANIMATION_ARRAY` in `Part.h`): walk, punches, kick,
  guard, run, dashes, deflects, pre/pos special Jyn and others.
- `Skill` (punch, kick, jyn) has damage, AP cost, energy balls and XP/levels from `skills.object`.
  `SpecialManager` spawns `SpecialInterface` subclasses (`SpecialPunch`, `SpecialKick`, `SpecialJyn`), which
  are physics projectiles using `EnergyParticle`s. The Jyn swarm moves using **Particle Swarm Optimization**
  (personal and global best, inertia, `AC1`/`AC2` in `SpecialInterface.h`), which is intentional.
  `SkillHit` / `SkillHitManager` show floating damage text.
- `AIManager` (singleton) → `RobotAI` → `RobotDefensiveAI` simulates key presses on the enemy.
- `Camera` is a third-person chase/lock camera with exploration and fight modes, configured by
  `camera.object`.
- `GUI` (singleton, about 1.4k lines) is all CEGUI: start menu, options, loading screen, battle HUD bars, ESC
  menu (Status / Inventory with a render-to-texture part preview / Skills / Help), conversations, and a dialog
  queue (Alert, Confirm, ShowPart). `Tutorial` is a guided tutorial state machine.
- `SoundManager` / `Sound` wrap OgreAL. `Sky` wraps SkyX or a simple skydome. `Clock` keeps the in-game time.
- `ConfigScriptLoader` (`ConfigScript.*`) is an Ogre `ScriptLoader` for `*.object` files. Look up a block with
  `ConfigScriptLoader::getSingleton().getConfigScript("<type>", "<name>")->findChild("key")->getValueI()`.
- `DotSceneLoader` is the Ogitor dotScene loader (with terrain and PagedGeometry) and uses `rapidxml.hpp`.
- `Util` has barrel spawning and shape updates. `Log` / `LogManager` provide the on-screen log.

## Data and assets (`media/`)

- `media/configuration/*.object` holds the **game tuning, data-driven**: `demo.object` (enemy count, hero
  and `enemyN` loadouts and stats), `game.object` (speed, mass, regen), `skills.object`, `camera.object` and
  `animation.object` (loop flags).
- `media/tumbu/robot00{1..5}/` holds one robot "set" each: `head/body/leftArm/rightArm/legs_00N.mesh` +
  `.skeleton`, textures in TGA (`*UV`, `NM*` normal, `SM*` specular, `GM*`/`AO*`), `robot00N.material`,
  `robot00N.object` (part stats and attach positions) and the source `robot00N.blend`.
- Meshes use MeshSerializer v1.41 and skeletons use Serializer v1.10 (Ogre 1.7 era). Models were exported
  from **Blender 2.49** with the old Python "Ogre Meshes Exporter" and OgreXMLConverter. That exporter no
  longer runs. For new exports use **blender2ogre** (OGRECave). Details are in DEV_SETUP.
- `media/tumbu/robots/` holds the shared Cg shaders (`general.cg`, `general.program`, `robots.material`).
- `media/scenes/arena/` holds `Arena.scene` (Ogitor) and `Terrain/Page_00000000.ogt`.
- `media/gui/` holds CEGUI 0.7 schemes, imagesets, looknfeel, fonts and layouts (`battle`, `menu`,
  `startScreen`, `conversation`, `dialogSystem`, `loading`). `RobotFaces.imageset` holds the portraits.
- `media/musics/*.ogg` holds `intro_music` and `battle_music`, and `media/sounds/*.ogg` holds punch, kick,
  walk and explosion.
- `media/packs/OgreCore.zip` and `SdkTrays.zip` are Ogre core resources. `media/SkyX/` has SkyX shaders
  (HLSL only).

## Conventions when editing

- Style: tabs, braces on the same line, `//----...` separators between methods, `getInstance()` singletons,
  raw `new`/`delete`, and `NULL`. Match this style until a deliberate modernization pass.
- **Encoding:** sources and some media text files are **Windows-1252 / Latin-1**, not UTF-8 (Portuguese
  accents in comments). Keep the encoding when editing, or convert whole files deliberately in a separate
  commit. Line endings are mixed CRLF/LF.
- New data or tuning belongs in `*.object` scripts rather than hardcoded values.
- Known portability blockers for modern compilers and other OSes:
  - `stdext::hash_map` in `ConfigScript` (removed or hard-deprecated in modern MSVC; use `std::unordered_map`).
  - Extra qualification `TUMBU::createSimpleRigidBody` inside the class in `TUMBU.h` (rejected under
    `/permissive-` and by GCC/Clang).
  - `TumbuEnums::PhysicObjectTag::TERRAIN` scoped use of an unscoped enum.
  - `ExpandEnvironmentStrings`, `SetClassLong` and `WinMain` are Win32-only, and non-Windows paths are only
    partly handled.
  - SkyX is compiled only under `_WINDOWS`.
  - Cg shaders.
- Known latent bugs to watch for: `Demo` never initialises `sky`, `camera` or `physicWorld` in its
  constructor, but the destructor checks them for NULL. `delete pDataConvert` should be `delete[]`. Collision
  dispatch is wrapped in `catch(...)`.

## Related material outside the repo

- `E:\WorkSpaces\workspace tcc\Tumbu\`: the old SVN working copy with **runnable `bin/Release` and `bin/Debug`**,
  `LEIAME.txt` (Portuguese readme and controls), and `tumbu.mpg` (a 2011 gameplay video).
- `E:\WorkSpaces\workspace tcc\11-03-01 Tumbu.rar`: a snapshot from March 2011.
- `D:\Backup\OgreSDK\`: `OGRE_addons/` (CEGUI-0.7.5, bullet-2.77, ogrebullet, ogreal, ogg, vorbis, skyx,
  pagedgeometry, vld, btogre), `OGRE_tools/` (Ogitor, OgreMeshy, ParticleEditor) and `OgreSDK_vc10_v1-8-1`.
- `D:\Backup\LINUX\DEV\ogre` + `ogre_build`: the Ogre **1.7.1** source and a Linux build (an early attempt at
  cross-platform). `D:\Backup\LINUX\workspace\TUMBU\`: a Linux-side copy with built exes.
- `E:\WorkSpaces\workspace games\OgreAL-Eihort`, `...\ogrebullet`: dependency source checkouts (SVN).
