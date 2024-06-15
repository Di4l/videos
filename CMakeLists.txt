# CMakeList.txt: proyecto de CMake para alulme, incluya el origen y defina
# la lógica específica del proyecto aquí.
#
#------------------------------------------------------------------------------
cmake_minimum_required (VERSION 3.10)
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
set(PROJECT_VERSION 0.0.1)
set(PROJECT_DESCRIPTION "OLC - Console Game Engine")
#------------------------------------------------------------------------------

project(olcCGE LANGUAGES CXX VERSION ${PROJECT_VERSION} DESCRIPTION ${PROJECT_DESCRIPTION})
#------------------------------------------------------------------------------

add_definitions(-DUNICODE)
add_definitions(-D_UNICODE)
#------------------------------------------------------------------------------

set(HEADER_FILES
	olcConsoleGameEngine.h
)

set(SOURCE_FILES
	olcConsoleGameEngine.cpp
)
#------------------------------------------------------------------------------

# Agregue un origen al ejecutable de este proyecto.
add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES} ${HEADER_FILES})
#------------------------------------------------------------------------------

set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 17
)
#------------------------------------------------------------------------------
