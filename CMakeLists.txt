cmake_minimum_required(VERSION 3.20)

project(bnk-extract-GUI LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(INSTALL_DOCS OFF)
set(INSTALL_PKG_CONFIG_MODULE OFF)
set(INSTALL_CMAKE_PACKAGE_MODULE OFF)

add_subdirectory(libogg-1.3.5)
set(CMAKE_OGG_FOUND TRUE)
set(OGG_LIBRARY ogg)
get_target_property(OGG_INCLUDE_DIR ogg INCLUDE_DIRECTORIES)
add_subdirectory(libvorbis-1.3.7)


file(GLOB_RECURSE BNK_EXTRACT_SRC CMAKE_CONFIGURE_DEPENDS
    bnk-extract/*.cpp
    bnk-extract/*.c
    bnk-extract/*.hpp
    bnk-extract/*.h
    )

file(GLOB BNK_EXTRACT_GUI_SRC CMAKE_CONFIGURE_DEPENDS *.c *.h *.rc)

add_executable(bnk-extract-GUI WIN32 ${BNK_EXTRACT_SRC} ${BNK_EXTRACT_GUI_SRC})
target_link_libraries(bnk-extract-GUI PRIVATE ogg vorbis vorbisenc vorbisfile)
target_link_libraries(bnk-extract-GUI PRIVATE ole32 uuid comctl32 gdi32 comdlg32 winmm)
