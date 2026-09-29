# TUMBU — Developer setup, step by step

This guide has four parts:

1. **Run the 2011 build as-is.** No compiling and no code changes. About 5 minutes.
2. **Rebuild with the original toolchain** (VC++ 2010 + Ogre 1.7). This reproduces the 2011 dev environment.
3. **Asset pipeline**: Blender, the exporters, Ogitor and the installer.
4. **Modernization roadmap**: what to replace, and in what order.

Paths below are the ones found on Jonathan's machine on 2026-09-28. Adjust them if things move.

---

## Part 1 — Run the existing 2011 build (no changes)

**Short answer: yes, it should run with no code or lib changes.** A complete Release build from Dec 2011, with
every DLL it needs, is in the old SVN working copy:

```
E:\WorkSpaces\workspace tcc\Tumbu\bin\Release\TUMBU.exe
```

Its `src/` and `include/` are identical to this repo. Its `media/` is also identical, apart from a few
`.blend` scratch files. This PC already has the runtimes it needs: the VC++ 2010 x86 runtime
(`C:\Windows\SysWOW64\msvcr100.dll`) and `d3dx9_43.dll`. OpenAL (`OpenAL32.dll` + `wrap_oal.dll`) and Cg
(`cg.dll`) ship next to the exe.

### Steps

1. **Create the per-user folder.** The game writes `ogre.cfg`, `ogre.log` and `cegui.log` there, and Ogre
   throws "Cannot create settings file" if the folder is missing. The installer used to create it.
   ```powershell
   New-Item -ItemType Directory -Force "$env:USERPROFILE\Tumbu"
   ```
2. **Run it from its own folder.** The working directory matters because media is loaded from `../../media`.
   ```powershell
   Set-Location "E:\WorkSpaces\workspace tcc\Tumbu\bin\Release"
   .\TUMBU.exe
   ```
   You can also double-click `TUMBU.exe` in Explorer, which uses the right working directory.
3. The **Ogre config dialog** appears the first time. Choose:
   - Render System: **Direct3D9 Rendering Subsystem** (the original readme recommends this)
   - Full Screen: **No** for the first try, Video Mode **1024 x 768**, VSync **Yes**

   Click OK. The choice is saved to `%USERPROFILE%\Tumbu\ogre.cfg`. Delete that file to see the dialog again.
4. Play. Controls: **WASD** to move, **1/U** punch, **2/I** kick, **3/O** special (Jyn), **Left Ctrl/P**
   guard, **Q/E** rotate the camera, the mouse moves the camera, **ESC** opens the menu, **Space** confirms.
   The game offers a tutorial at the start.

### If it does not start

- Read `%USERPROFILE%\Tumbu\ogre.log` first. It shows which plugin, resource or render system failed.
- **Direct3D9 fails:** delete `ogre.cfg` and pick the **OpenGL Rendering Subsystem**. Then try the reverse.
- **Missing `MSVCR100.dll` / `MSVCP100.dll`** (on another PC): install the *Microsoft Visual C++ 2010 SP1
  Redistributable **x86***. It must be x86 even on 64-bit Windows.
- **Missing `d3dx9_43.dll`** (on another PC): install the *DirectX End-User Runtime (June 2010)*.
- **Mouse gets captured:** OIS grabs the mouse in the foreground. Use Alt+Tab to get out.
- **"Sky quality: high" crashes or looks wrong:** that mode uses SkyX. Stay on low.
- **The Debug build** (`bin\Debug`) will **not** run on a normal PC. It needs the debug CRT (`MSVCR100D.dll`),
  which only Visual Studio 2010 installs. Use Release.

### Optional: run it from this repo instead

Copy the runtime files into this repo's `bin\Release`. `*.dll` and `*.exe` are git-ignored; the `.cfg` files
are not.

```powershell
$src = "E:\WorkSpaces\workspace tcc\Tumbu\bin\Release"
$dst = "E:\WorkSpaces\workspace games\Tumbu\bin\Release"
New-Item -ItemType Directory -Force $dst
Copy-Item "$src\*" $dst -Include *.dll,*.exe,plugins.cfg,tumbu.cfg
Set-Location $dst; .\TUMBU.exe
```

---

## Part 2 — Rebuild with the original toolchain (VC++ 2010 + Ogre 1.7)

**Why VC++ 2010 specifically?** Every prebuilt dependency (Ogre, CEGUI, OIS, OgreAL, SkyX, OgreBullet,
PagedGeometry) was compiled with VC++ 2010 (MSVCR100/MSVCP100). The C++ standard library ABI
(`std::string`, `std::vector`, and so on) changed with every MSVC version up to 2015. Code built with
VS2015 or newer **cannot link or run** against those DLLs. A Jan 2017 attempt set `PlatformToolset v140` in
`TUMBU.vcxproj`, which could not work. The project now uses `Windows7.1SDK`, the VC++ 2010 compiler.

You have two choices:
- **(A) Legacy rebuild.** Get the VC++ 2010 compiler and reuse the 2011 binaries. Steps are below.
- **(B) Rebuild every dependency with a modern compiler.** At that point, porting to modern libraries is
  better use of the time (Part 4).

### 2.1 Install the tools

This is the procedure that worked on Jonathan's machine (Windows 11) on 2026-09-28. VS2010 itself is no
longer offered on my.visualstudio.com; a free account only lists VS2010 runtimes. So the VC++ 2010
compiler comes from **Windows SDK 7.1**, which Microsoft still hosts.

1. **Visual Studio 2022 Community** with the *Desktop development with C++* workload. It provides the IDE
   and MSBuild. To install it without clicking through the installer:
   ```powershell
   winget install --id Microsoft.VisualStudio.2022.Community --override "--wait --passive --norestart --add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended"
   ```
2. **Download the VC++ 2010 compiler packages** (all links verified 2026-09-28):
   - Windows SDK 7.1, x64 ISO:
     `https://download.microsoft.com/download/F/1/0/F10113F5-B750-4969-A255-274341AC6BCE/GRMSDKX_EN_DVD.iso`
   - VC++ 2010 SP1 compiler update (KB2519277):
     `https://download.microsoft.com/download/7/5/0/75040801-126C-4591-BCE4-4CD1FD1499AA/VC-Compiler-KB2519277.exe`
   - VC++ 2010 SP1 redistributables, for putting back afterwards:
     `https://download.microsoft.com/download/1/6/5/165255E7-1014-4D0A-B094-B6A430A6BFFC/vcredist_x86.exe`
     and the matching `vcredist_x64.exe`.
3. **Install from an elevated PowerShell.** The SDK's `setup.exe` is unreliable on Windows 10/11, so skip it
   and install the individual MSIs from the mounted ISO. Run these steps in this order:
   1. Uninstall "Microsoft Visual C++ 2010 x86/x64 Redistributable". They block the compiler MSI.
      Use `msiexec /x {F0C3E5D1-1ADE-321E-8167-68EF0DE699A5}` for x86 and
      `msiexec /x {1D8E6291-B0D5-35EC-8441-6616F567A0F7}` for x64.
   2. Install, from `<ISO>:\Setup\`, with `msiexec /i … /qn`: `WinSDK_amd64\WinSDK_amd64.msi`,
      `WinSDKBuild_amd64\WinSDKBuild_amd64.msi`, `WinSDKTools_amd64\WinSDKTools_amd64.msi`,
      `WinSDKWin32Tools_amd64\WinSDKWin32Tools_amd64.msi` and `vc_stdx86\vc_stdx86.msi`.
   3. Run `VC-Compiler-KB2519277.exe /q /norestart`.
   4. Reinstall `vcredist_x86.exe /q` and `vcredist_x64.exe /q`. Until you do, old apps that need
      MSVCR100 (including the 2011 TUMBU.exe) will not start.
4. **Two post-install fixes** that a real VS2010 install would have handled:
   - Add `C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE` to the **user PATH**. It holds
     `mspdb100.dll`. Without it, `CL.exe` exits with `-1073741515` (0xC0000135, DLL not found).
     `scripts/build.ps1` also adds it on the fly.
   - The SP1 update's `intrin.h` includes `ammintrin.h`, but the update does not install that file on top
     of SDK 7.1. Create a stand-in (a header guard only; it declares AMD XOP/FMA4 intrinsics nobody here uses)
     at `C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\include\ammintrin.h`. This needs admin rights.
5. **Project toolset:** the project uses `PlatformToolset = Windows7.1SDK`, which pairs the VC++ 2010
   compiler with SDK 7.1. Plain `v100` looks up the VS2010-bundled SDK v7.0A in the registry and fails with
   `MSB8003 Could not find WindowsSDKDir`. On a machine with a real VS2010 + SP1, `v100` also works.
6. Optional: **NSIS 3.x** to build the installer, and **7-Zip** or WinRAR for the old archives.

### 2.2 Recreate `D:\OgreSDK` (`OGRE_HOME`)

Originally the SDK lived at `D:\OgreSDK`, with addons in `D:\OgreSDK\addons`. The Debug config still
hardcodes `D:\OgreSDK\addons\vld`. Recreating that layout avoids editing the project.

1. **Get the Ogre 1.7 VC10 SDK.** The game's DLLs are Ogre 1.7 "Cthugha", dated June 2011, so most likely
   **1.7.3**. Download `OgreSDK_vc10_v1-7-3.exe` (a self-extracting 7z, about 55 MB). The link was verified
   on 2026-09-28:
   `https://sourceforge.net/projects/ogre/files/ogre/1.7/OgreSDK_vc10_v1-7-3.exe/download`.
   Extract it and move its contents so that these paths exist:
   ```
   D:\OgreSDK\include\OGRE\Ogre.h
   D:\OgreSDK\include\OIS\OIS.h
   D:\OgreSDK\include\OGRE\SdkTrays.h      (SdkTrays / SdkCameraMan)
   D:\OgreSDK\lib\release\OgreMain.lib
   D:\OgreSDK\boost_1_44\                  (the project includes $(OGRE_HOME)\boost_1_44)
   D:\OgreSDK\bin\release\*.dll
   ```
   If the SDK has no `boost_1_44` folder, download Boost 1.44 separately and put it there.
   The backup at `D:\Backup\OgreSDK\OgreSDK_vc10_v1-8-1` is **Ogre 1.8.1**. It is ABI-incompatible with the
   1.7-built addons, so do not use it for this build.
2. **Restore the addons from backup.** All of them were built with VC10 on 2011-07-24 against the same Ogre:
   ```powershell
   Copy-Item -Recurse "D:\Backup\OgreSDK\OGRE_addons" "D:\OgreSDK\addons"
   ```
3. **Install the OpenAL 1.1 Core SDK.** Download `OpenAL11CoreSDK.zip` from
   `https://www.openal.org/downloads/OpenAL11CoreSDK.zip`, unzip it and run the installer. The installer
   has no silent mode, so click through it and keep the default location,
   `C:\Program Files (x86)\OpenAL 1.1 SDK`. The Creative SDK is required: OgreAL includes `xram.h`, which
   OpenAL Soft does not provide.

### 2.3 Set the environment variables

The project resolves every dependency from environment variables. Set them once (as user variables), then
**restart Visual Studio**:

```powershell
$A = "D:\OgreSDK\addons"
setx OGRE_HOME           "D:\OgreSDK"
setx BULLET_HOME         "$A\bullet-2.77"           # headers from \src
setx OGRE_BULLET_HOME    "$A\ogrebullet"            # \Collisions\include, \Dynamics\include, \lib\Release (also holds the Bullet .libs)
setx OGREAL_SDK          "$A\ogreal"                # \include, \lib\Release
setx OPENAL_SDK          "C:\Program Files (x86)\OpenAL 1.1 SDK"   # \include, \libs\Win32
setx LIB_OGG_HOME        "$A\ogg"
setx LIB_VORBIS_HOME     "$A\vorbis"
setx CEGUI_HOME          "$A\CEGUI-0.7.5"           # \cegui\include, \lib
setx SKYX_HOME           "$A\skyx"                  # \SkyX\SkyX\bin\SkyX.lib
setx PAGED_GEOMETRY_HOME "$A\pagedgeometry"         # \include, \lib\Release
```

Notes:
- SkyX headers come from `$(OGRE_HOME)\addons\skyx\SkyX\SkyX`, which is why the addons go inside `OGRE_HOME`.
- The linker gets the Bullet libs (`bulletcollision.lib`, `LinearMath.lib`, and so on) from
  `$(OGRE_BULLET_HOME)\lib\Release`, where the 2011 build copied them. `bullet-2.77\msvc\2008` holds
  **VC2008** builds; do not use those.
- Ogg and Vorbis only provide include paths. The game does not link them directly (OgreAL does).
- The project may reference CEGUI headers through `$(OGRE_HOME)\include\CEGUI`. That is harmless if the
  folder is missing, because `$(CEGUI_HOME)\cegui\include` is also on the include path.

### 2.4 Build

From the repo root, in any PowerShell:

```powershell
.\scripts\build.ps1                 # Release build → bin\Release\TUMBU.exe  (-Configuration Debug, -Rebuild)
.\scripts\run.ps1                   # copies runtime DLLs/cfgs, ensures %USERPROFILE%\Tumbu, launches the game
```

- `build.ps1` loads the dependency variables from the user environment, puts VS2010's `Common7\IDE` on
  the PATH, finds VS2022's MSBuild through `vswhere` and builds `TUMBU.sln` (Win32).
- `run.ps1` copies the version-matched runtime DLLs from the 2011 build
  (`E:\WorkSpaces\workspace tcc\Tumbu\bin\<Config>`; override with `$env:TUMBU_RUNTIME_DIR`). It refreshes
  `bin\<Config>\tumbu*.cfg` from the root `tumbu.cfg`, which is the source of truth, and then starts the
  exe with the right working directory.
- **In the IDE:** open `TUMBU.sln` in VS2022. If it offers to **retarget or upgrade**, choose **No**. Run
  `run.ps1` once so that the DLLs are in place, then press **F5**. `.vcxproj.user` already sets the debugger
  working directory to `bin\$(Configuration)`. Start VS after the environment variables and PATH are set,
  or restart it.
- Expected harmless log noise: `white.png` not found (the placeholder in `robots.material` is always
  overridden) and `city_6_*.dds` not found (a terrain layer in `Arena.scene` that was never in the project).
  The 2011 build logs the same messages.

**Debug build:** it links the `_d` libs, uses `plugins_d.cfg` / `tumbu_d.cfg`, needs the debug DLLs (copy them
from `E:\WorkSpaces\workspace tcc\Tumbu\bin\Debug`) and `vld_x86.dll` from `D:\OgreSDK\addons\vld\bin\Win32`.
In debug, **F12** dumps the scene graph, **G** shows the camera panel, **R** toggles wireframe, and physics
debug shapes are drawn.

### 2.5 Common build errors

| Error | Cause and fix |
|---|---|
| `Cannot open include file 'Ogre.h'` / `'CEGUI.h'` | An env var is unset or points at the wrong level. Restart VS after `setx`. |
| `LNK2019` on `std::basic_string` or Ogre symbols | Wrong toolset. It must be **Windows7.1SDK** or **v100**, not v140 or newer. |
| `MSB8003 Could not find WindowsSDKDir` | `v100` without a real VS2010. Use `Windows7.1SDK`. |
| `CL.exe` exited with code `-1073741515` | `mspdb100.dll` is not found. Add VS2010 `Common7\IDE` to the PATH (see 2.1 step 4). |
| `C1083: Cannot open include file: 'ammintrin.h'` | Missing file after the SP1 compiler update. Create the stand-in (see 2.1 step 4). |
| `LNK1104 OgreMain.lib` | `$(OGRE_HOME)\lib\Release` is missing, or you pointed at the 1.8.1 SDK. |
| `<hash_map> is deprecated` / `C4596 illegal qualified name` | You are compiling with v140 or newer. Use v100, or apply the modernization fixes in CLAUDE.md. |
| Linker cannot find `vld.lib` (Debug) | `D:\OgreSDK\addons\vld\lib\Win32` does not exist. Restore the addons. |

---

## Part 3 — Asset pipeline

### Blender models

- All robots, the arena, the gym, the house, the trees and the splash screen are **Blender 2.49** files.
  One exception: `media/tumbu/arena/coliseum.blend` was **re-saved in Blender 4.0.1** in Jul 2024, and that
  change is uncommitted. To restore the 2.49 original:
  `git checkout -- media/tumbu/arena/coliseum.blend` (also delete `coliseum.blend1`).
- Modern Blender (5.2 is installed) can **open** 2.49 files. Armatures and actions usually survive, but
  materials and some settings are converted. **Always "Save As" a new file.** Never overwrite the 2.49
  originals.
- The original exporter was the Blender 2.49 Python "Ogre Meshes Exporter" plus `OgreXMLConverter`. It no
  longer exists for current Blender.
- **The current exporter is blender2ogre** (github.com/OGRECave/blender2ogre), which supports current
  Blender. Check its README for Blender 5.x support.
- **Format compatibility:**
  - The legacy Ogre 1.7 game reads meshes up to v1.41. Modern `OgreMeshTool` writes newer versions that
    1.7 cannot load. To feed the legacy build, export `.mesh.xml` / `.skeleton.xml` with blender2ogre, then
    convert with the **old** tools in `D:\Backup\OgreSDK\OgreCommandLineTools_1.7.2.zip`
    (`OgreXMLConverter body_001.mesh.xml`).
  - Once the game moves to Ogre 14, use Ogre 14's `OgreMeshTool` and upgrade all existing meshes with it.
- Each robot part is its own mesh + skeleton: `head_00N`, `body_00N`, `leftArm_00N`, `rightArm_00N`,
  `legs_00N`. They must keep the **14 animation names** in `Part.h` / `animation.object`: `walk`,
  `right_punch`, `guard`, `no_pose`, `pre_special_jyn`, `pos_special_jyn`, `left_punch`, `right_kick`, `run`,
  `dash_back`, `dash_left`, `dash_right`, `left_up_deflect`, `right_up_deflect`.
- Attach positions and stats per part live in `media/tumbu/robot00N/robot00N.object`.

### Other tools (backed up in `D:\Backup\OgreSDK\OGRE_tools`)

- **Ogitor 0.4.4** was the scene editor that produced `media/scenes/arena/Arena.scene` and the terrain
  `.ogt`. Ogitor is abandoned. A modern replacement is Blender + blender2ogre's `.scene` export, or
  hand-editing the XML.
- **OgreMeshy** is a mesh viewer for checking exports.
- **ParticleEditor** was used to author `media/particle/*.particle`.

### Windows installer (NSIS)

`Tumbu.nsi` builds "Tumbu BETA Installer - 1.50.exe". The compiled installer was not found on any disk. To
rebuild it, install NSIS 3.x and create this staging layout next to the `.nsi`:

```
Tumbu\
  bin\Release\        TUMBU.exe + DLLs + plugins.cfg + tumbu.cfg + ogre.cfg (default video settings)
  media\
  dependencies\vcredist_x86.exe      (VC++ 2010 SP1 x86 redistributable)
  ShyDS Games.url
```

Then run `makensis Tumbu.nsi`. The installer installs to `Program Files\Shyds\Tumbu`, runs the vcredist,
creates `%USERPROFILE%\Tumbu` and copies `ogre.cfg` into it, and adds Start menu and desktop shortcuts plus an
uninstaller.

---

## Part 4 — Modernization roadmap (after the legacy build runs)

Recommended order. Each step should leave the game runnable.

1. **Freeze the legacy state.** Commit or revert the uncommitted `TUMBU.vcxproj` and `coliseum.blend`
   changes. Add `.vs/`, `obj/`, `*.VC.db`, `*.sdf`, `ipch/` and `*.blend1` to `.gitignore`. Tag the result,
   for example `legacy-2011`.
2. **Build system.** Move to **CMake + vcpkg**, targeting x64 and VS2022. vcpkg has `ogre` (14.x),
   `bullet3`, `ois`, `openal-soft`, `libvorbis` and `cegui`. That gives a single `cmake --preset` build and
   is the basis for Linux and macOS later.
3. **Compiler cleanups** (they can be done before the engine port):
   - replace `stdext::hash_map` with `std::unordered_map`
   - remove the extra `TUMBU::` qualifications in `TUMBU.h`
   - fix the uninitialised pointers in `Demo`
   - `delete[] pDataConvert`
   - replace `ExpandEnvironmentStrings` with a portable user-dir helper
4. **Ogre 1.7 → Ogre 14 (1.x line, not Ogre-Next).** Port the Root/config/resource setup, move to
   `OgreBites::ApplicationContext` / `TrayManager`, port the Terrain API, convert **Cg shaders to
   HLSL/GLSL** (or use the RTSS), switch from D3D9 to D3D11 or GL3+, and upgrade meshes with `OgreMeshTool`.
5. **Physics.** Drop the dead OgreBullet and use **Bullet 3** directly, through Ogre's own `Bullet`
   component (Ogre 13+) or a thin wrapper. The code already dispatches collisions by rigid-body name, so the
   interface (`CollisionDetectionListener`) can stay.
6. **Audio.** Replace OgreAL with **OpenAL Soft** plus a small wrapper, or with **miniaudio**. Only `Sound`
   and `SoundManager` touch it.
7. **GUI.** CEGUI 0.7 → **Dear ImGui** (Ogre ships `ImGuiOverlay`), MyGUI, or CEGUI 0.8.7. This is the
   biggest port (`GUI.cpp`, about 1.4k lines, plus 6 layouts).
8. **Sky and vegetation.** Replace SkyX with a skydome or skybox shader (low-quality mode already works
   without it). Take PagedGeometry from OGRECave/ogre-pagedgeometry, or drop it.
9. **Input.** Keep OIS (the maintained fork is wgois/OIS) or move to SDL2 through OgreBites, which also
   provides gamepads cross-platform.
10. **Cross-platform.** Once 2–9 are done, the remaining Win32 code is `WinMain`, the icon code and the
    user-dir path. Linux and macOS then mostly come from CMake.
