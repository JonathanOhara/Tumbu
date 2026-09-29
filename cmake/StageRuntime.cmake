# Copies everything TUMBU.exe needs at runtime into its folder (run as a post-build step).
#   -DDEPS_INSTALL=<deps>/install  -DSOURCE_DIR=<repo>  -DBIN_DIR=<repo>/bin/<Config>

# Runtime DLLs of Ogre, SDL2, MyGUI, Caelum...
file(GLOB _dlls "${DEPS_INSTALL}/bin/*.dll")
foreach(_dll IN LISTS _dlls)
	get_filename_component(_name "${_dll}" NAME)
	file(COPY_FILE "${_dll}" "${BIN_DIR}/${_name}" ONLY_IF_DIFFERENT)
endforeach()

# Ogre's media: core shaders/materials (Main), the shader generator library, terrain shaders and the tray UI pack.
foreach(_dir Main RTShaderLib Terrain)
	file(COPY "${DEPS_INSTALL}/Media/${_dir}" DESTINATION "${BIN_DIR}/OgreMedia")
endforeach()
file(COPY "${DEPS_INSTALL}/Media/packs/SdkTrays.zip" DESTINATION "${BIN_DIR}/OgreMedia/packs")

# MyGUI's base media (skins, fonts, pointers), when MyGUI is installed.
if(EXISTS "${DEPS_INSTALL}/share/MYGUI/Media/MyGUI_Media")
	file(COPY "${DEPS_INSTALL}/share/MYGUI/Media/MyGUI_Media" DESTINATION "${BIN_DIR}")
endif()

# Caelum's media (day/night sky), when Caelum is installed.
if(EXISTS "${DEPS_INSTALL}/share/Caelum/Media")
	file(COPY "${DEPS_INSTALL}/share/Caelum/Media/" DESTINATION "${BIN_DIR}/CaelumMedia")
endif()

# Config files owned by the repo.
foreach(_cfg plugins.cfg resources.cfg)
	file(COPY_FILE "${SOURCE_DIR}/${_cfg}" "${BIN_DIR}/${_cfg}" ONLY_IF_DIFFERENT)
endforeach()
