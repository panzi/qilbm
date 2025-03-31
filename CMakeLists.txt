cmake_minimum_required(VERSION 3.26)

project(QILBM)

# ---- dependencies -----------------------------------------------------------
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 REQUIRED COMPONENTS Gui)
find_package(KF6FileMetaData)

find_package(ECM 6.11.0  NO_MODULE)
if (${ECM_FOUND})
	set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})
	include(KDEInstallDirs)
	include(KDECompilerSettings)
	include(KDECMakeSettings)
endif()

find_package(PkgConfig REQUIRED)

# ---- compiler flags ---------------------------------------------------------
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -Wall -Wextra -pedantic -Werror -Wno-write-strings -Wno-suggest-override -O3")
	add_compile_options(-std=c++20 -Wall -Wextra -pedantic -Werror -Wno-write-strings -Wno-suggest-override -O3)
endif()

# ---- build library ----------------------------------------------------------
qt_add_plugin(QILBM PLUGIN_TYPE imageformats)
set_property(TARGET QILBM PROPERTY CXX_STANDARD 20)
target_sources(QILBM PRIVATE src/QILBM.cpp src/ILBM.cpp src/Palette.cpp)
target_link_libraries(QILBM Qt6::Gui)

if(KF6FileMetaData_FOUND)
	add_library(KILBM)
	set_target_properties(KILBM PROPERTIES PREFIX "")
	target_sources(KILBM PRIVATE src/KILBM.cpp src/ILBM.cpp src/Palette.cpp)
	target_link_libraries(KILBM KF6::FileMetaData)
endif()

# ---- install target ---------------------------------------------------------
install(TARGETS QILBM DESTINATION ${QT6_INSTALL_PLUGINS}/imageformats)

if(KF6FileMetaData_FOUND)
	install(TARGETS KILBM DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/kfilemetadata)
endif()
