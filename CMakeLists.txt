# TUMBU on Ogre 14 (Visual Studio 2022, x64).
#
# Dependencies are built once with scripts/deps.ps1 into TUMBU_DEPS_DIR (default D:/TumbuDeps/modern).
# Build:   cmake --preset vs2022   then   cmake --build --preset release     (or open build/vs2022/TUMBU.sln)
# The executable and everything it needs at runtime end up in bin/<Config>.
cmake_minimum_required(VERSION 3.21)
project(TUMBU VERSION 2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Every dependency is a Release build (see scripts/deps.ps1), so the game offers Release and RelWithDebInfo
# (optimised, with debug info and the in-game developer tools: TUMBU_DEBUG).
set(CMAKE_CONFIGURATION_TYPES "Release;RelWithDebInfo" CACHE STRING "" FORCE)
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

#------------------------------------------------------------------------------ dependencies
if(DEFINED ENV{TUMBU_DEPS_DIR})
	set(_deps_default "$ENV{TUMBU_DEPS_DIR}")
else()
	set(_deps_default "D:/TumbuDeps/modern")
endif()
set(TUMBU_DEPS_DIR "${_deps_default}" CACHE PATH "Folder where scripts/deps.ps1 built the dependencies")
file(TO_CMAKE_PATH "${TUMBU_DEPS_DIR}" TUMBU_DEPS_DIR)
set(TUMBU_DEPS_INSTALL "${TUMBU_DEPS_DIR}/install")
list(APPEND CMAKE_PREFIX_PATH "${TUMBU_DEPS_INSTALL}" "${TUMBU_DEPS_DIR}/ogredeps")

find_package(OGRE 14.6 REQUIRED COMPONENTS Bites Overlay Terrain Paging RTShaderSystem Bullet CONFIG
	PATHS "${TUMBU_DEPS_INSTALL}/CMake" NO_DEFAULT_PATH)
find_package(Bullet REQUIRED CONFIG PATHS "${TUMBU_DEPS_DIR}/ogredeps/lib/cmake/bullet" NO_DEFAULT_PATH)

# Imported Ogre targets only have a Release configuration; use it for RelWithDebInfo too.
set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO "Release")

#------------------------------------------------------------------------------ game
file(GLOB TUMBU_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB TUMBU_HEADERS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/include/*.h" "${CMAKE_SOURCE_DIR}/include/*.hpp")

add_executable(TUMBU WIN32 ${TUMBU_SOURCES} ${TUMBU_HEADERS} src/MiniaudioImpl.c res/TUMBU.rc)

target_include_directories(TUMBU PRIVATE
	"${CMAKE_SOURCE_DIR}/include"
	"${TUMBU_DEPS_INSTALL}/include/bullet"
	"${TUMBU_DEPS_INSTALL}/include/miniaudio")

target_link_directories(TUMBU PRIVATE "${TUMBU_DEPS_DIR}/ogredeps/lib")
target_link_libraries(TUMBU PRIVATE
	OgreMain OgreBites OgreOverlay OgreTerrain OgrePaging OgreRTShaderSystem OgreBullet
	BulletDynamics BulletCollision LinearMath)

target_compile_definitions(TUMBU PRIVATE
	$<$<CONFIG:RelWithDebInfo>:TUMBU_DEBUG>
	_CRT_SECURE_NO_WARNINGS NOMINMAX)

if(MSVC)
	# /utf-8: sources are UTF-8 (or ASCII); /MP: parallel compilation.
	target_compile_options(TUMBU PRIVATE /utf-8 /MP /W3 /wd4244 /wd4267 /wd4305)
endif()

set(TUMBU_BIN_DIR "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>")
set_target_properties(TUMBU PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/Release"
	RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/bin/RelWithDebInfo"
	VS_DEBUGGER_WORKING_DIRECTORY "${TUMBU_BIN_DIR}")
set_property(DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT TUMBU)
source_group(TREE "${CMAKE_SOURCE_DIR}" FILES ${TUMBU_SOURCES} ${TUMBU_HEADERS})

#------------------------------------------------------------------------------ runtime files next to the exe
# DLLs, Ogre's own media and the config files are copied after every build (only when changed).
add_custom_command(TARGET TUMBU POST_BUILD
	COMMAND "${CMAKE_COMMAND}"
		-DDEPS_INSTALL=${TUMBU_DEPS_INSTALL}
		-DSOURCE_DIR=${CMAKE_SOURCE_DIR}
		-DBIN_DIR=$<TARGET_FILE_DIR:TUMBU>
		-P "${CMAKE_SOURCE_DIR}/cmake/StageRuntime.cmake"
	COMMENT "Staging runtime files in bin/$<CONFIG>"
	VERBATIM)
