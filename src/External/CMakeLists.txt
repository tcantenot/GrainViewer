# This file is part of GrainViewer, the reference implementation of:
#
#   Michel, Élie and Boubekeur, Tamy (2020).
#   Real Time Multiscale Rendering of Dense Dynamic Stackings,
#   Computer Graphics Forum (Proc. Pacific Graphics 2020), 39: 169-179.
#   https://doi.org/10.1111/cgf.14135
#
# Copyright (c) 2017 - 2020 -- Télécom Paris (Élie Michel <elie.michel@telecom-paris.fr>)
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the “Software”), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# The Software is provided “as is”, without warranty of any kind, express or
# implied, including but not limited to the warranties of merchantability,
# fitness for a particular purpose and non-infringement. In no event shall the
# authors or copyright holders be liable for any claim, damages or other
# liability, whether in an action of contract, tort or otherwise, arising
# from, out of or in connection with the software or the use or other dealings
# in the Software.

###############################################################################
# Hide warnings for all targets defined here in External
###############################################################################

# Not the most elegant way to do so, and not portable, but this is a notoriously
# hard thing to do with cmake...
if (MSVC)
	message("Hiding warnings for all targets defined here in External.")
	set(NEW_FLAGS "")
	separate_arguments(CMAKE_CXX_FLAGS)
	foreach(FLAG ${CMAKE_CXX_FLAGS})
		if(NOT FLAG MATCHES "/W[0-4]")
			list(APPEND NEW_FLAGS ${FLAG})
		endif()
	endforeach()
	list(APPEND NEW_FLAGS "/w")
	string(REPLACE ";" " " CMAKE_CXX_FLAGS "${NEW_FLAGS}")
endif(MSVC)

###############################################################################
# Submodules
###############################################################################

# Glad (v0.1.27), providing headers for OpenGL procedures
# Here it is configured to link to OpenGL 4.5 Core profile, but you can change
# the two following lines to get another one. If you do so, you must also
# change the window creation flags.
set(GLAD_PROFILE "core" CACHE STRING "" FORCE)
set(GLAD_API "gl=4.5,gles2=" CACHE STRING "" FORCE)
add_subdirectory(glad)
set_property(TARGET glad PROPERTY FOLDER "External")
set_property(TARGET glad-generate-files PROPERTY FOLDER "External")

add_subdirectory(modernglad)
set_property(TARGET modernglad PROPERTY FOLDER "External")
set_property(TARGET modernglad-generate-files PROPERTY FOLDER "External")

# GLFW (3.2.1), for openning the window and getting mouse/keyboard events
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)
set_property(TARGET glfw PROPERTY FOLDER "External")

# Dear ImGui
add_library(imgui STATIC 
	imgui/imgui.h
	imgui/imgui.cpp
	imgui/imgui_widgets.cpp
	imgui/imgui_demo.cpp
	imgui/imgui_draw.cpp
	imgui/examples/imgui_impl_glfw.h
	imgui/examples/imgui_impl_glfw.cpp
	imgui/examples/imgui_impl_opengl3.h
	imgui/examples/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC imgui imgui/examples)
add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLAD)
target_link_libraries(imgui LINK_PRIVATE glad glfw)
set_property(TARGET imgui PROPERTY FOLDER "External")

# glm
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE glm)

# rapidjson
add_library(rapidjson INTERFACE)
target_include_directories(rapidjson INTERFACE rapidjson/include)

# TinyObjLoader
add_subdirectory(tinyobjloader)
set_property(TARGET tinyobjloader PROPERTY FOLDER "External")

# TinyEXR
add_library(tinyexr STATIC 
	tinyexr/tinyexr.h
	tinyexr/tinyexr.cc
)
target_include_directories(tinyexr PUBLIC tinyexr)
set_property(TARGET tinyexr PROPERTY FOLDER "External")

###############################################################################
# Repackaged third party libraries
###############################################################################

add_subdirectory(tinygltf)
add_subdirectory(nanoflann)
add_subdirectory(refl-cpp)
