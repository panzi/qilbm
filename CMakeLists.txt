cmake_minimum_required(VERSION 3.16)

project(QILBM)

# ---- dependencies -----------------------------------------------------------

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 REQUIRED COMPONENTS Gui)

find_package(ECM 6.11.0  NO_MODULE)
if (${ECM_FOUND})
	set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})
	include(KDECompilerSettings)
endif()

find_package(PkgConfig REQUIRED)

# ---- compiler flags ---------------------------------------------------------
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wextra -pedantic -Werror -Wno-write-strings -Wno-suggest-override -O3")
endif()

include_directories(${VTFLIB_INCLUDE_DIRS})

if(NOT(${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
	add_definitions(-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE)
endif()

# ---- build library ----------------------------------------------------------
qt_add_plugin(QILBM PLUGIN_TYPE imageformats)
target_sources(QILBM PRIVATE src/QILBM.cpp src/ILBM.cpp src/Palette.cpp)
target_link_libraries(QILBM Qt6::Gui ${VTFLIB_LIBRARIES})

# ---- install target ---------------------------------------------------------
install(TARGETS QILBM DESTINATION ${QT6_INSTALL_PLUGINS}/imageformats)
