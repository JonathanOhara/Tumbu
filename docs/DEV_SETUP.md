# TUMBU — Developer setup, step by step

The game was ported from the 2011 stack (Ogre 1.7, VC++ 2010, 32-bit) to **Ogre 14.6, VS2022, CMake, x64**
in Sep 2026 (see [MODERNIZATION_PLAN.md](MODERNIZATION_PLAN.md)). This guide has these parts:

- **Part A: build and run the modern version** (branch `master`). This is the one to use.
- **Part B: asset pipeline** (Blender, exporters, meshes, installer).
- **Legacy parts 1 and 2:** run or rebuild the 2011 version (git tag `legacy-2011`). Keep them for reference.

Paths below are the ones on Jonathan's machine in Sep 2026. Adjust them if things move.

---

## Part A — Modern build (Ogre 14.6 + Visual Studio 2022)

### A.1 Install the tools (once)

1. **Visual Studio 2022** (Community is fine) with the **Desktop development with C++** workload. It
   includes MSVC, the Windows SDK and "C++ CMake tools for Windows". The scripts use the CMake that ships
   inside Visual Studio, so no separate CMake install is needed.
   ```powershell
   winget install --id Microsoft.VisualStudio.2022.Community --override "--wait --passive --norestart --add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended"
   ```
2. **Git.** `curl.exe` and `tar.exe` come with Windows 10/11, and the dependency script uses them.
3. **PowerShell 7** (`pwsh`) to run the scripts: the maintained version, and the one they are tested with.
   Windows PowerShell 5.1 (the `powershell` Windows opens by default) also works: the scripts that pipe Blender's
   or Ogre's output relax `$ErrorActionPreference` around those calls, because 5.1 turns anything a native tool
   prints to stderr into an error. Keep that pattern in new scripts.
   ```powershell
   winget install --id Microsoft.PowerShell --source winget
   ```
4. **Python 3.14** (installed 2026-10-05, per-user, `%LOCALAPPDATA%\Programs\Python\Python314`, with the `py`
   launcher) for helper scripts and one-off checks outside Blender. The Blender scripts in `scripts/blender/` use
   Blender's own Python; without a Python install, `blender --background --python script.py` also runs a plain
   script.
   ```powershell
   winget install --id Python.Python.3.14 --exact --source winget --scope user
   ```
5. Nothing else. The Ogre 1.7 SDK, the OpenAL SDK, the env variables and the VC++ 2010 compiler from the
   legacy parts are **not** needed.

### A.2 Build the dependencies (once, about 20–40 minutes)

```powershell
.\scripts\deps.ps1                  # everything: ogre, mygui, caelum, miniaudio
.\scripts\deps.ps1 -Only caelum     # rebuild a single package
```

- It downloads pinned sources (Ogre 14.6.0, MyGUI 3.5.1, Caelum master, miniaudio 0.11.25) and builds them
  in Release, x64. Ogre also builds its own dependencies (SDL2, Bullet 3, FreeType, pugixml) as static libs.
- Everything goes to `D:\TumbuDeps\modern` (`src`, `build`, `ogredeps`, `install`), outside the repo. To use
  another folder, set the user env variable `TUMBU_DEPS_DIR` before running `deps.ps1`, then restart
  Visual Studio. The game's CMake reads the same variable.
- To rebuild Ogre's own dependencies from scratch, delete `ogredeps` first. Ogre only builds them when that
  folder does not exist.
- The script patches two upstream problems in the downloaded sources:
  - an Ogre 14.6 terrain memory leak (`Repair-OgreSource`)
  - Caelum's shaders for Direct3D 11 (`Repair-CaelumShaders`)

  Both patches are idempotent, so running the script again is safe.

### A.3 Build and run from the command line

```powershell
.\scripts\build.ps1                               # Release → bin\Release\TUMBU.exe
.\scripts\build.ps1 -Configuration RelWithDebInfo # debug info + debug keys (TUMBU_DEBUG)
.\scripts\build.ps1 -Clean                        # regenerate the solution and rebuild everything
.\scripts\run.ps1                                 # start the game from bin\Release
```

The build's post-build step (`cmake/StageRuntime.cmake`) copies everything the exe needs next to it:
- the DLLs
- `OgreMedia/`, `MyGUI_Media/` and `CaelumMedia/`
- `plugins.cfg` and `resources.cfg`

The repo's `media/` is used in place (`../../media`), so edits to materials, layouts and `*.object` files
need no rebuild. Restart the game to see them.

### A.4 Build, run and debug in Visual Studio

1. Build the dependencies once (A.2).
2. Generate the solution. Either run `.\scripts\build.ps1` once, or run `cmake --preset vs2022` in a
   "Developer PowerShell for VS 2022".
3. Open **`build\vs2022\TUMBU.sln`** in Visual Studio 2022.
4. In the toolbar, pick the configuration:
   - **Release**: the normal game.
   - **RelWithDebInfo**: optimized, with PDBs, so breakpoints and the call stack work. It also defines
     `TUMBU_DEBUG` (debug keys, physics debug drawing). There is no `Debug` configuration, because the
     dependencies are Release-only and must share one C runtime.
5. `TUMBU` is the startup project, and its debugger working directory is already set to
   `bin\<Configuration>`. Press **F5** to build and debug, or **Ctrl+F5** to run without the debugger.
6. To pass test switches (A.6), use **Project → Properties → Debugging → Command Arguments**.

Alternative: **File → Open → Folder** on the repo root. Visual Studio reads `CMakePresets.json` directly.
Pick the `vs2022` preset and `TUMBU.exe` as the startup item.

Adding a source file: put it in `src/` or `include/`. CMake globs these folders, so re-run the configure
step (`build.ps1 -Clean`, or reload the CMake project in VS) to pick up new files.

### A.5 First run and settings

- The per-user folder `%USERPROFILE%\Tumbu\` is created automatically. It holds:
  - `ogre.cfg`: render system and video mode. Delete it to see the Ogre config dialog again.
  - `options.cfg`: in-game options (`SkyQuality`, `Shadows`, `FrameLimit`).
  - `ogre.log` and `MyGUI.log`.
- **Render system:** **Direct3D 11** is the default and recommended. **OpenGL 3+** also works, but the
  Caelum sky needs Direct3D 11, so on OpenGL "High" sky quality falls back to the simple skydome.
- **Frame rate:** the default is **VSync**, which runs at the monitor's refresh rate (144 FPS on a 144 Hz
  screen). In **Options → Frame limit** you can choose VSync, 144, 72, 60 or Unlimited. Movement and physics
  are frame-rate independent, so the game plays the same at any FPS.
- **Sky quality:**
  - **Low** is a static skydome.
  - **High** is the Caelum day/night sky (sun, moon, stars, clouds) driven by the game's `Clock`.
- **Controls:**
  - WASD to move, Left Shift to run.
  - **U/1** kick, **O/2** punch, **I/3** special (Jyn). Left Ctrl/P to guard.
  - Q/E rotate the camera, and PageUp/PageDown/Home/End zoom and tilt it.
  - ESC opens the menu or cancels, and Space/Enter confirms.
  - Gamepad (SDL2): Y special, X punch, A kick/OK, LB run, RB guard, Start menu, B cancel.
- **Debug keys (RelWithDebInfo only):** G camera panel, R wireframe/points, F5 reload textures, Print
  Screen screenshot (saved to `%USERPROFILE%\Tumbu`), Y kills the hero, F12 quits.

### A.6 Automated checks

Run these after every change. They start the game with no human input, write a screenshot and a log, and
quit.

```powershell
.\scripts\devtest.ps1 -QuitAfter 7 -Name check   # auto-start a match, walk the hero, screenshot, quit
.\scripts\devtest.ps1 -FpsCap 30 -Name fps30     # same at 30 FPS (compare movement across frame rates)
.\scripts\devtest.ps1 -Hour 22 -Name night       # start the clock at 22:00 (check the night sky; needs Sky quality High)
```

- The output goes to `%USERPROFILE%\Tumbu\devtest-<Name>.png` and `.log`. The log has
  `[DEVTEST] t=… fps=… hero=x y z enemy=x y z` every 0.5 s. The arena is about ±9 units, so any robot
  outside that range is a bug.
- `TUMBU.exe -guitour` walks through every GUI screen and saves `devtest-gui-*.png`: start menu, options,
  dialogs, HUD, ESC menu, inventory. It then quits to the menu, plays a second match and leaves with
  Exit. Clicks are real mouse events through the game's input dispatch.
- **Memory leak check:** `TUMBU.exe -cycles=12` plays 12 matches through the menus, with 3 enemy kills each,
  then Quit. After each match it logs a `[DEVTEST] memory …` line to `ogre.log`.
  - Healthy: `heap=` (bytes really allocated) stays flat from the second match on, and every object count
    (nodes, entities, materials, textures, meshes, widgets) returns to the same value each time.
  - `private=` (Windows' figure) moves around by a few MB. That's driver memory and fragmentation, not a
    leak.
- The raw switches are `-autoplay`, `-walktest`, `-guitour`, `-cycles=N`, `-fpscap=N`, `-quitafter=S` and
  `-hour=H`.
- **Crash reports:** when the game crashes, the call stack is written to `%USERPROFILE%\Tumbu\crash.log`
  and to `ogre.log`. Build RelWithDebInfo to get file and line numbers for the game's code.
- The reported FPS follows VSync, so it equals the monitor's current refresh rate. Windows can switch a
  144 Hz screen to 60 Hz; check it in Settings → Display → Advanced display.

### A.7 Troubleshooting

| Problem | Cause and fix |
|---|---|
| CMake: `Could not find OGRE` / `MyGUI` / `Caelum` | Dependencies are not built, or `TUMBU_DEPS_DIR` points elsewhere. Run `deps.ps1`. |
| Launching `TUMBU.exe` fails with **"Access denied"** (Acesso negado), or the process hangs at about 2 MB | The antivirus (**Norton 360** on this PC) holds freshly built, unknown exes. Add an exclusion for the repo's `bin\` folder (Norton → Settings → Antivirus → Scans and Risks → Items to Exclude from Scans / Auto-Protect), or allow the file when Norton asks. |
| `deps.ps1` download or git fails with a TLS/SSL error | Norton intercepts HTTPS. For git, use `git -c http.sslBackend=schannel …`. The scripts avoid git clones. |
| Pink/white or missing materials | Look in `ogre.log` for "Cannot locate resource". Media must be listed in `resources.cfg`. Game media that engine lookups need (skydome, fonts) lives in `[General]`. |
| `Program '…' is not supported` in `ogre.log` | A shader failed to compile, and the material silently fell back to its simpler technique. Read the error below that line. Direct3D 11 compiles `hlsl` at shader model 2 level unless the program sets `target vs_4_0` / `ps_4_0`. |
| The High sky is black, flat yellow or missing | Caelum's media must come from `deps.ps1`, which patches its shaders for Direct3D 11 (`Repair-CaelumShaders`). Rerun `.\scripts\deps.ps1 -Only caelum`, then build. |
| A mesh fails with "unsupported mesh version" | It is an Ogre 1.7 mesh. Upgrade it with `scripts\upgrade-meshes.ps1`. |
| Crash right at start | Delete `%USERPROFILE%\Tumbu\ogre.cfg` and pick Direct3D 11 again. Then check `ogre.log`. |
| Any other crash | Read `%USERPROFILE%\Tumbu\crash.log` (the call stack). Reproduce it in a RelWithDebInfo build for file:line. |

Harmless log noise: `city_6_*.dds` not found. It is a terrain layer in `Arena.scene` that was never in the
project.

---

## Part B — Asset pipeline

### Blender models

- **Rule: Blender is the source of truth for geometry.** Never edit a `.mesh`/`.skeleton` directly (by hand,
  through its XML, or by a script that patches it). Make every change in the `.blend` file, then export
  again with blender2ogre. Put scripted fixes in the Blender build scripts so they are reproducible.
  Working files live in `art/` (`art/arena/Arena.blend` for the arena). A model without one gets one there
  first, made from its 2011 `.blend`; the original is never overwritten. Once a working file has been edited
  by hand, it is the only source: `bake-arena-ao.ps1 -Rebuild` would replace it, so do not use `-Rebuild`
  after manual edits.
- All robots, the arena, the gym, the house, the trees and the splash screen are **Blender 2.49** files
  (Blender 5.2 also reports `media/tumbu/arena/coliseum.blend` as 2.49).
- **The `.blend` files match the game's `.mesh` files** (checked Sep 2026: identical triangle counts and
  sizes for the coliseum, the arena floor in `gym.blend` and the robot001 parts). Ogre has more vertices
  only because it splits them at UV seams. In the `.blend` the robot parts sit on the armature; each exported
  part `.mesh` is re-centred on its own origin, and the attach points in `robot00N.object` rely on that.
- Blender 5.2 (installed) **opens** 2.49 files with meshes, UVs, materials and action names, but:
  - **it no longer converts pre-2.50 animation data** ("Open & save the file with Blender v4.5"). Robot
    files with animations must go through **Blender 4.5 LTS** once (open, save as a new file);
  - the 2011 `robot002.blend` crashes Blender 5.2 on load (access violation);
  - **Done for the robots:** `.\scripts\convert-legacy-blend.ps1 -Robots` used the portable Blender 4.5.14
    in `D:\TumbuDeps\tools\blender-4.5.14-windows-x64\` (no installer: unzip from
    download.blender.org/release/Blender4.5/) to write `art/robots/robot00N.blend`. All 14 animations of
    each robot survived, and Blender 5.2 opens all five, robot002 included. The same script converts any
    other 2.49 file (`-Source … -Target art\…`) and refuses to overwrite an existing working file;
  - material node groups are not read (harmless: the game's materials are hand-written `.material` files).
  **Always "Save As" a new file.** Never overwrite the 2.49 originals.
- The original exporter was the Blender 2.49 Python "Ogre Meshes Exporter" plus `OgreXMLConverter`. It no
  longer exists for current Blender.
- **The current exporter is blender2ogre** (github.com/OGRECave/blender2ogre), import and export of
  `.mesh`/`.skeleton`/`.scene`. Installed in Blender 5.2 (Sep 2026, master commit `0d094a4`, recorded in
  `%APPDATA%\Blender Foundation\Blender\5.2\scripts\addons\io_ogre\BLENDER2OGRE_COMMIT.txt`), with its
  converter set to `D:\TumbuDeps\modern\install\bin\OgreXMLConverter.exe`. Its releases only state support
  up to Blender 4.4, but static meshes export correctly from 5.2: the coliseum re-export is identical to
  the game's file. **Animated (skinned) meshes need Blender 4.5 LTS**: blender2ogre's animation export reads the
  action API that Blender 5 removed (`Action.groups`). The same commit is installed for the portable 4.5 in
  `%APPDATA%\Blender Foundation\Blender\4.5\scripts\addons\io_ogre` (copied from the 5.2 folder); the robot export
  sets the converter path itself. See "Robot export" below.
  **Before upgrading Blender** (or blender2ogre), check that blender2ogre's animation export works with the new
  version: run `.\scripts\export-robots.ps1 -Blender <new blender.exe>` (no `-Save`/`-Install`, so nothing changes);
  every robot must report "all parts match Blender" and compare at zero with the installed files. Only then switch.
  The blocker is small (`ogre/skeleton.py` reads `action.groups` / `action.fcurves`, removed in Blender 5); a fix could
  go upstream to OGRECave/blender2ogre.
  - Scripted (headless) use: `blender.exe --background file.blend --python script.py`, then in the script
    `from io_ogre import api; api.dot_mesh(obj, out_dir, overwrite=True)`.
- **Mesh formats:** the modern build uses Ogre 14 meshes. All 53 meshes and skeletons were upgraded with
  `scripts/upgrade-meshes.ps1`:
  1. The old 1.7 tool converts each binary 1.7 file to XML.
  2. `OgreXMLConverter` from `D:\TumbuDeps\modern\install\bin` converts the XML to Ogre 14.

  New blender2ogre exports only need step 2, or nothing at all if blender2ogre writes the binary mesh itself.
  The legacy 1.7 build cannot load these meshes. It reads meshes up to v1.41, so use the files in the
  `legacy-2011` tag for it.
- Each robot part is its own mesh + skeleton: `head_00N`, `body_00N`, `leftArm_00N`, `rightArm_00N`,
  `legs_00N`. They must keep the **14 animation names** in `Part.h` / `animation.object`: `walk`,
  `right_punch`, `guard`, `no_pose`, `pre_special_jyn`, `pos_special_jyn`, `left_punch`, `right_kick`, `run`,
  `dash_back`, `dash_left`, `dash_right`, `left_up_deflect`, `right_up_deflect`.
- Attach positions and stats per part live in `media/tumbu/robot00N/robot00N.object`.
- Robot materials derive from `media/tumbu/robots/robots.material` and set their textures with material
  variables (`set $diffuseMap …`). The shaders are HLSL + GLSL (`robot_*.vert/.frag`, unified through
  `OgreUnifiedShader.h`).

### Arena ambient occlusion (`art/arena/Arena.blend`)

- `.\scripts\bake-arena-ao.ps1` runs `scripts/blender/arena_ao.py` in headless Blender 5.2:
  1. first run or `-Rebuild`: creates `art/arena/Arena.blend` from `coliseum.blend` and `gym.blend`, adds a
     second UV map `AO` to each mesh (lightmap layout: seams only on hard edges and at 45/135/225/315
     degrees around the arena, so each smooth wall is one continuous island and its AO has no steps), and
     names the materials as the game does;
  2. every run: bakes Cycles AO through the `AO` UV map into `media/tumbu/arena/<mesh>_ao.png` (every
     object in the scene occludes) and exports `<mesh>.mesh` with blender2ogre (two UV sets).
- The build also cleans the 2011 meshes (`clean_mesh`): it stitches T-junctions within 2 cm (the coliseum had six,
  hairline cracks on the upper ring), removes loose edges, and marks edges sharper than 30 degrees as hard.
  Every face used to be smooth-shaded, so the window jambs and flat walls showed diagonal gradients.
- **Big shapes** (`scripts/blender/arena_shapes.py`, arena remake 2026-10): a plinth course at the foot of the inner
  arena wall (0.9 high, 0.15 out), a 0.08 chamfer on every hard edge of the coliseum (each chamfer edge hard, so the
  sun draws a flat band along it) and weighted normals (face area, hard edges kept; without them a vertex where a
  chamfer ends bent the normal of the curved wall and the toon ramp drew a bright wedge). It edits `Arena.blend` in
  place and records the steps it applied in the object property `tumbu_shapes`, so running it again only adds newer
  steps: `blender --background art/arena/Arena.blend --python scripts/blender/arena_shapes.py -- .`, then
  `.\scripts\bake-arena-ao.ps1`. The AO UV map is laid out again for the new faces by the bake.
- **Custom normals survive the export:** `arena_ao.py` hands blender2ogre triangles through a temporary Triangulate
  modifier with `keep_custom_normals` (blender2ogre's own triangulation loses them), as the robot export does.
- **Axis conversion is set explicitly:** blender2ogre keeps its settings between runs (`io_ogre.json`), and the robot
  export sets `SWAP_AXIS 'xyz'`; with it the arena exported lying on its side (Z-up). `arena_ao.py` sets `xz-y` before
  every export. Compare the bounding boxes of new exports with the old meshes (`OgreXMLConverter` to XML).
- **AO textures use clamp addressing** (`tex_address_mode clamp` in `Tumbu/EnvironmentToon`).
  The UV packing can place an island against the texture border; with the default wrap, bilinear filtering blended
  texels from the opposite border into it, which drew a thin dark vertical line on the upper ring along a
  seam lying on the border. The bake also starts from a white image, so filtering near UV islands drifts
  towards "unoccluded" rather than black.
- Collections: **Export** (baked and exported) and **Occluders** (geometry that only casts AO, such as a
  terrain proxy). Settings are custom properties, so they can be changed in Blender: scene
  `tumbu_ao_distance` (1 unit) and `tumbu_ao_samples` (1024), object `tumbu_ao_size` (texture pixels: 4096 for the coliseum, 1024 for the floor).
- In the game, `Tumbu/EnvironmentToon` reads `$aoMap` with `tex_coord_set 1`. How strongly AO darkens the
  ambient light and the sun, and its tint towards the shadow colour, are `aoAmbient` / `aoDirect` /
  `aoTint` in `lighting.object`.
- **Up axis:** the 2011 files disagree. `coliseum.blend` and the robot files are **Y-up** (Ogre's
  convention, exported without axis conversion); `gym.blend` is Z-up. blender2ogre converts Blender Z-up to
  Ogre Y-up, so Y-up sources must be turned upright (+90 degrees around X) first, or they export lying on
  their side. `arena_ao.py` does this for the coliseum. Check the bounding box of every new export against
  the old mesh.
- blender2ogre exports only **selected** objects (its `SELECTED_ONLY` setting) and silently skips the
  others: select the object before calling `api.dot_mesh`.

### Robot export (`art/robots/robot00N.blend` -> `media/tumbu/robot00N/`)

The robots' `.mesh` and `.skeleton` files are generated from the Blender files, which are the source of truth: whatever
an animator sees in Blender is what the game plays.

- `.\scripts\export-robots.ps1 [-Robot 1,3] [-Save] [-Install]` runs `scripts/blender/robot_export.py` in headless
  **Blender 4.5** (`D:\TumbuDeps\tools\blender-4.5.14-windows-x64`) for each robot:
  1. **Source fixes** (written back into the `.blend` with `-Save`; a second run reports "source already clean"):
     scene at exactly 30 fps; vertex groups that are no bone removed (2011 leftovers such as `body_L`, `Group`,
     `Spin1`: they deform nothing); weights normalised, then those under 1 % dropped (keeping at least one), at most
     4 per vertex (Ogre's limit), normalised again, repeated until stable; each part's material named as the game's
     material (`<part>UV_00N/TEXFACE/<part>UV_00N.tga`).
  2. **Export** of the five parts with blender2ogre: no axis conversion (the robot files are Y-up), only the bones each
     action moves as tracks, every frame baked; the 14 actions come from temporary NLA strips over each action's frame
     range (a manual frame range wins). Quads are split along the **shorter diagonal** (as the 2011 exporter did; it
     decides how shading is interpolated across a quad), through a temporary Triangulate modifier that also keeps the
     **custom normals** (blender2ogre's own bmesh triangulation loses them). **Tangents** are generated by Ogre
     (`OgreMeshUpgrader -t`): Blender cannot store tangents, and Ogre's match the 2011 files closely.
     **Skeleton in the part's space:** each part mesh is centred on its own origin (the attach points in
     `robot00N.object` rely on it), but blender2ogre writes the skeleton in the armature's space, so the script moves
     the root bones' rest positions by the offset between the two origins in the exported XML. Never do this by moving
     the bones in edit mode: leaving edit mode rebuilds the bone orientations from head, tail and roll, and for the
     2011 bones that flips some rolls by 180 degrees, so every animation changes. An action that moves no bone
     (`no_pose`, the rest pose) is not exported; the game treats it as optional (`Part.cpp`).
  3. **Checks against Blender** (`robot_check.py`), or nothing is installed: every exported vertex has the bones and
     weights of its Blender vertex and the normal of a Blender face corner with its position and UV; every bone at
     every frame of every action is where Blender puts it (the game side computed from the exported files the way
     Ogre poses a skeleton; verified against Ogre's own poses with `TUMBU.exe -posedump`).
  4. **Comparison with the files the game uses now** (`robot_compare.py`): the skinned position of every vertex at
     every frame of every animation, old against new. With `-Install` the new files are copied into
     `media/tumbu/robot00N/`.
- After installing, verify as usual: before/after shots, `TUMBU.exe -posedump` before and after (every bone of every
  part, animation and frame, as Ogre poses them, in `posedump.txt`), `-cycles`, and a fight benchmark.
- **One-time migration (2026-10-05)**, `scripts/blender/robot_migrate_2011.py`: the 2011 exporter computed its own
  vertex normals (they gave the robots their soft shading and differ from Blender's by about 4 degrees, up to 90), and
  robot003's and robot005's right arm played a 20-frame `right_punch` while the action has 30 (the punch fires at half
  the animation). The script copied the 2011 normals into the parts as custom normals (each face corner gets the normal
  of the 2011 vertex with its position and UV) and gave those two actions the manual frame range 1-21. Result: the
  export reproduces the 2011 game (bones within 0.0005 units, close-up images at the level of two identical runs).
  It is kept for reference; do not run it again (it reads the 2011 `.mesh` files, which are gone).
- **Editing robots in Blender:** keep the 14 action names and the part object names (`head_00N` ...); a new vertex
  needs a weight on at least one bone; normals are custom normals now (Blender's "Clear Custom Split Normals" returns
  to computed ones, which changes the shading); set an action's frame range when it should play shorter than its keys.

### Robot ambient occlusion (`art/robots/robot00N.blend`)

- `.\scripts\bake-robot-ao.ps1 [-Robot 1,3]` runs `scripts/blender/robot_ao.py` in headless Blender 5.2 and
  writes `media/tumbu/robot00N/AO<part>UV_00N.png` (for example `AOlegsUV_001.png`), the size of the part's
  diffuse texture. `robot00N.material` uses them through `$aoMap`.
- **It runs in Blender 4.5** (the default `-Blender`, `D:\TumbuDeps\tools\blender-4.5.14-windows-x64\blender.exe`): the
  bake saves the robot `.blend`, and those files must stay in Blender 4.5, which the robot export needs (blender2ogre's
  animation export does not work on Blender 5; see "Robot export"). Checked 2026-10-05 on robot001: 4.5 bakes the same
  AO as 5.2 did, apart from bake noise and the eye slot of the head, which now follows the custom normals the parts
  carry since the robot export (a hint darker at its edges; not visible in the game).
- The bake uses the parts' **shading normals**, so after changing a robot's normals bake it again.
- The bake goes through each part's **texture UVs** (their overlap is 0–7%), so the `.mesh` files are not
  re-exported. Each part occludes **only itself**, in the **rest pose**: parts are swapped between robots and
  animated, so occlusion from neighbouring parts would be wrong most of the time.
- Settings are scene custom properties of each robot file: `tumbu_ao_distance` (0.25 units) and
  `tumbu_ao_samples` (512).
- The 2011 hand-made `AOheadUV_001.tga` is kept, but no longer used.

### Other tools (backed up in `D:\Backup\OgreSDK\OGRE_tools`)

- **Ogitor 0.4.4** was the scene editor that produced `media/scenes/arena/Arena.scene` and the terrain
  `.ogt`. Ogitor is abandoned. A modern replacement is Blender + blender2ogre's `.scene` export, or
  hand-editing the XML.
- **OgreMeshy** is a mesh viewer for checking exports.
- **ParticleEditor** was used to author `media/particle/*.particle`.

### Windows installer (NSIS)

`Tumbu.nsi` builds "Tumbu BETA Installer - 1.50.exe". The compiled installer was not found on any disk. The
script is still the 2011 one: it installs 32-bit files and runs the VC++ 2010 redistributable. For the modern
build, update it to the layout below: x64, the new media folders and `vc_redist.x64.exe`. Then install
NSIS 3.x, create this staging layout next to the `.nsi`, and run `makensis Tumbu.nsi`:

```
Tumbu\
  bin\Release\        TUMBU.exe + DLLs + OgreMedia\ MyGUI_Media\ CaelumMedia\ + plugins.cfg + resources.cfg
  media\
  dependencies\vc_redist.x64.exe     (current VC++ 2015–2022 x64 redistributable)
  ShyDS Games.url
```

The installer installs to `Program Files\Shyds\Tumbu`, runs the vcredist, creates `%USERPROFILE%\Tumbu`, and
adds Start menu and desktop shortcuts plus an uninstaller.

---

## Legacy part 1 — Run the existing 2011 build (no changes)

A complete Release build from Dec 2011, with every DLL it needs, is in the old SVN working copy. It runs
with no code or lib changes:

```
E:\WorkSpaces\workspace tcc\Tumbu\bin\Release\TUMBU.exe
```

It needs the VC++ 2010 x86 runtime (`msvcr100.dll`) and `d3dx9_43.dll`, both present on this PC. OpenAL and
Cg ship next to the exe.

1. Create the per-user folder: `New-Item -ItemType Directory -Force "$env:USERPROFILE\Tumbu"`. Ogre throws
   "Cannot create settings file" without it.
2. Run it from its own folder (media is loaded from `../../media`). Double-clicking in Explorer also works.
3. In the Ogre config dialog, choose **Direct3D9**, windowed, 1024 x 768, VSync on.

If it does not start:
- Read `%USERPROFILE%\Tumbu\ogre.log`.
- Try the OpenGL render system.
- On another PC, install the *VC++ 2010 SP1 Redistributable x86* and the *DirectX End-User Runtime (June
  2010)*.

Its movement speed depends on the frame rate, which was fixed in the modern code. On a 144 Hz screen the
hero crawls and enemies fly off.

---

## Legacy part 2 — Rebuild with the original toolchain (VC++ 2010 + Ogre 1.7)

These steps apply only to the `legacy-2011` tag (`git checkout legacy-2011`). The modern branch has no
`TUMBU.sln` / `.vcxproj` any more.

**Why VC++ 2010:** every prebuilt dependency (Ogre, CEGUI, OIS, OgreAL, SkyX, OgreBullet, PagedGeometry)
was compiled with VC++ 2010 (MSVCR100). The C++ standard library ABI changed with every MSVC version up to
2015, so newer compilers cannot link against those DLLs.

### Tools

VS2010 is no longer offered for download, so the VC++ 2010 compiler comes from **Windows SDK 7.1**
(links verified 2026-09-28):
- SDK 7.1 x64 ISO:
  `https://download.microsoft.com/download/F/1/0/F10113F5-B750-4969-A255-274341AC6BCE/GRMSDKX_EN_DVD.iso`
- VC++ 2010 SP1 compiler update (KB2519277):
  `https://download.microsoft.com/download/7/5/0/75040801-126C-4591-BCE4-4CD1FD1499AA/VC-Compiler-KB2519277.exe`
- VC++ 2010 SP1 redistributables:
  `https://download.microsoft.com/download/1/6/5/165255E7-1014-4D0A-B094-B6A430A6BFFC/vcredist_x86.exe`
  (and `vcredist_x64.exe`)

Install from an elevated PowerShell. Skip the SDK's `setup.exe`, which is unreliable on Windows 10/11.
1. Uninstall the VC++ 2010 x86/x64 redistributables. They block the compiler MSI:
   `msiexec /x {F0C3E5D1-1ADE-321E-8167-68EF0DE699A5}` and `msiexec /x {1D8E6291-B0D5-35EC-8441-6616F567A0F7}`.
2. From `<ISO>:\Setup\`, install with `msiexec /i … /qn`:
   - `WinSDK_amd64\WinSDK_amd64.msi`
   - `WinSDKBuild_amd64\WinSDKBuild_amd64.msi`
   - `WinSDKTools_amd64\WinSDKTools_amd64.msi`
   - `WinSDKWin32Tools_amd64\WinSDKWin32Tools_amd64.msi`
   - `vc_stdx86\vc_stdx86.msi`
3. Run `VC-Compiler-KB2519277.exe /q /norestart`, then reinstall both redistributables with `/q`.
4. Add `C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE` to the user PATH. It holds
   `mspdb100.dll`; without it, `CL.exe` exits with -1073741515.
5. Create a header-guard-only stand-in for `ammintrin.h` in `...\Microsoft Visual Studio 10.0\VC\include`.
   The SP1 update's `intrin.h` includes it, but the update doesn't install it.
6. The project uses `PlatformToolset = Windows7.1SDK`. Do not retarget it to v140 or newer.

### Dependencies and environment

- Ogre 1.7.3 VC10 SDK in `D:\OgreSDK`, from
  `https://sourceforge.net/projects/ogre/files/ogre/1.7/OgreSDK_vc10_v1-7-3.exe/download`.
  It must include `boost_1_44`.
- The addons are restored from `D:\Backup\OgreSDK\OGRE_addons` to `D:\OgreSDK\addons`.
- The OpenAL 1.1 Core SDK installs to `C:\Program Files (x86)\OpenAL 1.1 SDK`.
- User environment variables:

```powershell
$A = "D:\OgreSDK\addons"
setx OGRE_HOME "D:\OgreSDK";  setx BULLET_HOME "$A\bullet-2.77";  setx OGRE_BULLET_HOME "$A\ogrebullet"
setx OGREAL_SDK "$A\ogreal";  setx OPENAL_SDK "C:\Program Files (x86)\OpenAL 1.1 SDK"
setx LIB_OGG_HOME "$A\ogg";   setx LIB_VORBIS_HOME "$A\vorbis";  setx CEGUI_HOME "$A\CEGUI-0.7.5"
setx SKYX_HOME "$A\skyx";     setx PAGED_GEOMETRY_HOME "$A\pagedgeometry"
```

### Build

On the `legacy-2011` checkout, `.\scripts\build.ps1` builds `TUMBU.sln` (Win32) with VS2022's MSBuild and
the VC++ 2010 compiler. `.\scripts\run.ps1` copies the 2011 runtime DLLs and launches the game. In the IDE,
open `TUMBU.sln` and answer **No** when it offers to retarget. Then press F5.
