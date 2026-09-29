# Builds the modern (Ogre 14) dependencies of TUMBU with Visual Studio 2022, x64.
#
#   .\scripts\deps.ps1                      # everything
#   .\scripts\deps.ps1 -Only ogre           # one package: ogre | mygui | caelum | miniaudio
#
# Everything is built in Release. Ogre builds its own dependencies (SDL2, Bullet, FreeType, pugixml) as
# static Release libraries, so the game links Release libraries in every configuration (Release and
# RelWithDebInfo) and all modules share one C runtime.
#
# Output: $DepsDir\install (headers, libs, DLLs, CMake configs). The game's CMake finds it through the
# TUMBU_DEPS_DIR environment variable (default D:\TumbuDeps\modern). Sources and build trees live in
# $DepsDir\src and $DepsDir\build, so they never touch the repo.
param(
    [string]$DepsDir = $(if ($env:TUMBU_DEPS_DIR) { $env:TUMBU_DEPS_DIR } else { 'D:\TumbuDeps\modern' }),
    [ValidateSet('all', 'ogre', 'mygui', 'caelum', 'miniaudio')] [string]$Only = 'all',
    [string[]]$Configs = @('Release')
)
$ErrorActionPreference = 'Stop'

# Pinned versions.
$OgreVersion      = '14.6.0'
$MyGuiVersion     = '3.5.1'
$CaelumCommit     = 'master'
$MiniaudioVersion = '0.11.25'

$src     = Join-Path $DepsDir 'src'
$build   = Join-Path $DepsDir 'build'
$install = Join-Path $DepsDir 'install'
# Ogre builds SDL2, Bullet, FreeType and pugixml (static, Release) here; the folder must not exist beforehand.
$ogreDeps = Join-Path $DepsDir 'ogredeps'
New-Item -ItemType Directory -Force $src, $build, $install | Out-Null

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath  = & $vswhere -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmake   = Join-Path $vsPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
if (-not (Test-Path $cmake)) { throw "CMake not found in Visual Studio ($vsPath). Install the 'C++ CMake tools for Windows' component." }
$generator = @('-G', 'Visual Studio 17 2022', '-A', 'x64')

function Invoke-Checked([string]$exe, [string[]]$arguments) {
    Write-Host ">> $exe $($arguments -join ' ')" -ForegroundColor DarkGray
    & $exe @arguments
    if ($LASTEXITCODE -ne 0) { throw "Command failed with exit code $LASTEXITCODE" }
}

function Get-Source([string]$name, [string]$url, [string]$folder) {
    $dir = Join-Path $src $folder
    if (Test-Path $dir) { return $dir }
    $archive = Join-Path $src "$name.tar.gz"
    Write-Host "Downloading $name" -ForegroundColor Cyan
    Invoke-Checked 'curl.exe' @('-sSL', '--fail', '-o', $archive, $url)
    Invoke-Checked 'tar.exe' @('-xzf', $archive, '-C', $src)
    return $dir
}

function Build-CMake([string]$name, [string]$sourceDir, [string[]]$options) {
    $buildDir = Join-Path $build $name
    Write-Host "Configuring $name" -ForegroundColor Cyan
    Invoke-Checked $cmake (@('-S', $sourceDir, '-B', $buildDir) + $generator + @(
        "-DCMAKE_INSTALL_PREFIX=$install", "-DCMAKE_PREFIX_PATH=$install;$ogreDeps") + $options)
    foreach ($config in $Configs) {
        Write-Host "Building $name ($config)" -ForegroundColor Cyan
        Invoke-Checked $cmake @('--build', $buildDir, '--config', $config, '--parallel')
        Invoke-Checked $cmake @('--install', $buildDir, '--config', $config)
    }
}

# Caelum's fragment programs skip the POSITION input that their vertex programs output first. Direct3D 9
# matched stage inputs by semantic, but Direct3D 11 matches them by register, so every input read the wrong
# value (the sky came out flat yellow). Give each fragment program a leading POSITION input.
function Repair-CaelumShaders([string]$mediaDir) {
    foreach ($file in Get-ChildItem (Join-Path $mediaDir '*.cg')) {
        $lines = [System.Collections.Generic.List[string]](Get-Content $file)
        $changed = $false
        for ($i = 0; $i -lt $lines.Count; $i++) {
            if ($lines[$i] -notmatch '^\s*void\s+\w*(FP|_fp)\s*$') { continue }
            $open = $i + 1
            while ($lines[$open] -notmatch '^\s*\(') { $open++ }
            if ($lines[$open + 1] -match ':\s*POSITION') { continue }
            $lines.Insert($open + 1, '    in float4 d3d11Position : POSITION,')
            $changed = $true
        }
        if ($changed) {
            Set-Content -Path $file -Value $lines
            Write-Host "Caelum: D3D11 input fix in $($file.Name)" -ForegroundColor DarkGray
        }
    }
}

# Ogre 14.6 bug: Terrain::prepare() allocates mDeltaData, and for version-1 terrain files (like the arena's
# 2011 Ogitor page) allocates it again before reading, leaking 4 * size^2 bytes (1 MB) on every terrain load.
function Repair-OgreSource([string]$sourceDir) {
    $file = Join-Path $sourceDir 'Components\Terrain\src\OgreTerrain.cpp'
    $text = [IO.File]::ReadAllText($file)
    $leak = "            // Load delta data`n            mDeltaData = OGRE_ALLOC_T(float, numVertices, MEMCATEGORY_GEOMETRY);`n"
    if ($text.Contains($leak)) {
        $text = $text.Replace($leak, "            // Load delta data (TUMBU patch: the buffer was already allocated above; allocating again leaked it)`n")
        [IO.File]::WriteAllText($file, $text)
        Write-Host "Ogre: patched the terrain delta-data leak in OgreTerrain.cpp" -ForegroundColor DarkGray
    }
}

if ($Only -in 'all', 'ogre') {
    $dir = Get-Source "ogre-$OgreVersion" "https://github.com/OGRECave/ogre/archive/refs/tags/v$OgreVersion.tar.gz" "ogre-$OgreVersion"
    Repair-OgreSource $dir
    Build-CMake 'ogre' $dir @(
        # Ogre downloads and builds SDL2, Bullet, FreeType and pugixml into $ogreDeps (only if it doesn't exist).
        '-DOGRE_BUILD_DEPENDENCIES=ON', "-DOGRE_DEPENDENCIES_DIR=$ogreDeps", '-DCMAKE_BUILD_TYPE=Release',
        '-DOGRE_BUILD_SAMPLES=OFF', '-DOGRE_BUILD_TESTS=OFF', '-DOGRE_BUILD_TOOLS=ON',
        '-DOGRE_INSTALL_SAMPLES=OFF', '-DOGRE_INSTALL_DOCS=OFF', '-DOGRE_INSTALL_PDB=ON',
        '-DOGRE_BUILD_COMPONENT_BITES=ON', '-DOGRE_BUILD_COMPONENT_OVERLAY=ON', '-DOGRE_BUILD_COMPONENT_OVERLAY_IMGUI=ON',
        '-DOGRE_BUILD_COMPONENT_TERRAIN=ON', '-DOGRE_BUILD_COMPONENT_PAGING=ON', '-DOGRE_BUILD_COMPONENT_RTSHADERSYSTEM=ON',
        '-DOGRE_BUILD_COMPONENT_BULLET=ON', '-DOGRE_BUILD_COMPONENT_MESHLODGENERATOR=ON',
        '-DOGRE_BUILD_COMPONENT_VOLUME=OFF', '-DOGRE_BUILD_COMPONENT_PROPERTY=OFF',
        '-DOGRE_BUILD_COMPONENT_PYTHON=OFF', '-DOGRE_BUILD_COMPONENT_JAVA=OFF', '-DOGRE_BUILD_COMPONENT_CSHARP=OFF',
        '-DOGRE_BUILD_PLUGIN_ASSIMP=OFF', '-DOGRE_BUILD_PLUGIN_EXRCODEC=OFF', '-DOGRE_BUILD_PLUGIN_FREEIMAGE=OFF',
        '-DOGRE_BUILD_PLUGIN_GLSLANG=OFF', '-DOGRE_BUILD_PLUGIN_CG=OFF', '-DOGRE_BUILD_PLUGIN_DOT_SCENE=OFF',
        '-DOGRE_BUILD_PLUGIN_PCZ=OFF', '-DOGRE_BUILD_PLUGIN_BSP=OFF',
        # Direct3D9 would need the discontinued 2010 DirectX SDK; Direct3D11 and OpenGL cover Windows.
        '-DOGRE_BUILD_RENDERSYSTEM_D3D9=OFF', '-DOGRE_BUILD_RENDERSYSTEM_D3D11=ON',
        '-DOGRE_BUILD_RENDERSYSTEM_GL=ON', '-DOGRE_BUILD_RENDERSYSTEM_GL3PLUS=ON',
        '-DOGRE_BUILD_RENDERSYSTEM_VULKAN=OFF', '-DOGRE_BUILD_RENDERSYSTEM_GLES2=OFF', '-DOGRE_BUILD_RENDERSYSTEM_TINY=OFF')
}

if ($Only -in 'all', 'mygui') {
    $dir = Get-Source "mygui-$MyGuiVersion" "https://github.com/MyGUI/mygui/archive/refs/tags/v$MyGuiVersion.tar.gz" "mygui-$MyGuiVersion"
    Build-CMake 'mygui' $dir @(
        '-DMYGUI_RENDERSYSTEM=3',                # 3 = Ogre (1.x)
        '-DMYGUI_USE_SYSTEM_PUGIXML=ON',         # reuse the pugixml Ogre built (no git clone)
        "-DOGRE_DIR=$install\CMake",
        '-DMYGUI_BUILD_DEMOS=OFF', '-DMYGUI_BUILD_TOOLS=OFF', '-DMYGUI_BUILD_DOCS=OFF',
        '-DMYGUI_BUILD_UNITTESTS=OFF', '-DMYGUI_BUILD_TEST_APP=OFF', '-DMYGUI_BUILD_WRAPPER=OFF',
        '-DMYGUI_STATIC=OFF')
    # MyGUI's base media (skins, fonts, pointers) is not installed without the demos; the game ships it.
    $media = Join-Path $install 'share\MYGUI\Media'
    New-Item -ItemType Directory -Force $media | Out-Null
    Copy-Item -Recurse -Force (Join-Path $dir 'Media\MyGUI_Media') $media
    # The game uses the BlackBlue theme (Media\Common\Themes); ship it inside MyGUI_Media.
    Copy-Item -Force (Join-Path $dir 'Media\Common\Themes\MyGUI_BlackBlue*') (Join-Path $media 'MyGUI_Media')
}

# Caelum on Ogre 14: generateSphericDome() looks for its dome mesh with resourceExists(name), which only
# searches global-pool groups, never the "Caelum" group the mesh lives in. The second match with the High sky
# then created the mesh again and Ogre threw "already exists". Look in Caelum's own group.
function Repair-CaelumSource([string]$sourceDir) {
    $file = Join-Path $sourceDir 'main\src\InternalUtilities.cpp'
    $text = [IO.File]::ReadAllText($file)
    $check = 'if (Ogre::MeshManager::getSingleton ().resourceExists (name)) {'
    if ($text.Contains($check)) {
        $text = $text.Replace($check, 'if (Ogre::MeshManager::getSingleton ().resourceExists (name, RESOURCE_GROUP_NAME)) {')
        [IO.File]::WriteAllText($file, $text)
        Write-Host "Caelum: patched the dome mesh lookup in InternalUtilities.cpp" -ForegroundColor DarkGray
    }
}

if ($Only -in 'all', 'caelum') {
    $dir = Get-Source "caelum-$CaelumCommit" "https://github.com/OGRECave/ogre-caelum/archive/$CaelumCommit.tar.gz" "ogre-caelum-$CaelumCommit"
    Repair-CaelumSource $dir
    Build-CMake 'caelum' $dir @("-DOGRE_DIR=$install\CMake")
    # Caelum's shaders, textures and meshes are not installed; the game ships them as CaelumMedia.
    $media = Join-Path $install 'share\Caelum\Media'
    New-Item -ItemType Directory -Force $media | Out-Null
    Copy-Item -Force (Join-Path $dir 'main\resources\*') $media
    Repair-CaelumShaders $media
}

if ($Only -in 'all', 'miniaudio') {
    # Single-file library: the game compiles miniaudio.c itself.
    $dir = Get-Source "miniaudio-$MiniaudioVersion" "https://github.com/mackron/miniaudio/archive/refs/tags/$MiniaudioVersion.tar.gz" "miniaudio-$MiniaudioVersion"
    $target = Join-Path $install 'include\miniaudio'
    New-Item -ItemType Directory -Force $target | Out-Null
    # stb_vorbis (bundled with miniaudio) decodes the game's .ogg files.
    Copy-Item (Join-Path $dir 'miniaudio.h'), (Join-Path $dir 'miniaudio.c'), (Join-Path $dir 'extras\stb_vorbis.c') $target -Force
    Write-Host "miniaudio $MiniaudioVersion -> $target" -ForegroundColor Cyan
}

Write-Host "Dependencies ready in $install" -ForegroundColor Green
