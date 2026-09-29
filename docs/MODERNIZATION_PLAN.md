# TUMBU — Modernization plan

Goal: move from the 2011 stack (Ogre 1.7.3, VC++ 2010, 32-bit) to **Ogre 14.6** on a current toolchain
(VS2022, CMake, x64). Upgrade one thing at a time, and after every step build and run the game to test it.
The dependency research behind these decisions is summarized below. The legacy build stays reachable
through the git tag `legacy-2011`.

## Decisions (made by Jonathan, 2026-09-29)

| Area | 2011 | Replacement | Why |
|---|---|---|---|
| Engine | Ogre 1.7.3 | **Ogre 14.6.0** (latest release, 2026-09-09) | Ogre 1.x is actively maintained |
| Physics | Bullet 2.77 + OgreBullet (dead) | **Ogre 14's built-in `Bullet` component** + Bullet 3 | Covers trimesh, heightfield terrain, compound shapes, collision listeners and debug draw |
| Input | OIS | **SDL2 through OgreBites** | Modern gamepads, and the input path Ogre 14 is built around |
| Shaders | Cg (`general.cg`) | **Hand-ported HLSL + GLSL** | Same look, and it runs on D3D11/GL3+ |
| GUI | CEGUI 0.7.5 (dead) | **MyGUI 3.5** (Ogre platform). Fallback: Dear ImGui | Retained widgets, layouts and skins map closely to the CEGUI design |
| Audio | OgreAL (dead) + OpenAL | **miniaudio** | Single header, no dependencies, built-in 3D audio |
| Sky | SkyX 0.2 (dead) | **Caelum** (OGRECave, needs Ogre 14.5+) | Day/night cycle, sun, moon, stars and clouds; fits `Clock` and the MORNING/NIGHT design |
| Vegetation | PagedGeometry | **Removed** | The arena never used it |
| Scene | own `DotSceneLoader` (Ogitor) | Keep ours, ported | It knows the Ogitor terrain format |
| Debug UI | SdkTrays | `OgreBites::TrayManager` | Same thing, renamed |

## Steps

Phase A runs on the legacy toolchain (`scripts/build.ps1`). Phase B uses the new CMake build.

- [x] **A1** Frame-rate-independent movement (hero crawls and enemy flies away at high FPS)
- [x] **A2** Code cleanups that both compilers accept (uninitialised pointers, `delete[]`, header qualification, `hash_map`)
- [x] **A3** Remove PagedGeometry
- [x] **A4** Port `general.cg` to HLSL + GLSL and drop the Cg plugin
- [x] **A5** Remove SkyX (the skydome serves both quality levels until Caelum is added)
- [x] **B1** CMake + VS2022 x64 + Ogre 14.6 build of the dependencies, and the project skeleton
- [x] **B2** App shell on Ogre 14: Root, window, resources, camera, OgreBites trays, SDL2 input
- [x] **B3** Scene loader + terrain on Ogre 14
- [x] **B4** Physics on the Ogre Bullet component
- [x] **B5** Audio on miniaudio
- [x] **B6** GUI on MyGUI (start menu, HUD, dialogs, ESC menu and inventory)
- [x] **B7** Mesh upgrade (scripts/upgrade-meshes.ps1) and modern render systems: Direct3D 11 (default) and OpenGL 3+
- [x] **C2** Sky on Caelum ("High" sky quality, Direct3D 11; OpenGL falls back to the skydome). This step
  also fixed the robot shaders, which since B7 had silently used their fallback technique. See "Rendering
  pitfalls" in CLAUDE.md.
- [x] **Frame rate** VSync by default (the monitor's rate, e.g. 144 Hz), with options for 144/72/60/Unlimited
- [x] **Done** Full game loop plays on Ogre 14.6, with docs for building in Visual Studio (DEV_SETUP Part A)

(C1, "input on SDL2", is folded into B2, because SDL2 comes with OgreBites.)

## Testing each step

The game has a developer command line (added in A1) so that every step can be checked without playing by
hand. It can skip the menu, start a match, write screenshots and logs, and quit on its own. After each step:
build, run the automated check, look at the screenshot and the logs, then commit.
