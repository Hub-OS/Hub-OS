# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

set(SFML_REPO ${CMAKE_CURRENT_SOURCE_DIR}/../../extern/SFML)

include_directories(${SFML_REPO}/extlibs/headers/freetype2 ${SFML_REPO}/extlibs/headers)

set(OPENAL_LIBRARY ${SFML_REPO}/extlibs/libs-android/${ANDROID_ABI}/libopenal.so)
set(OPENAL_INCLUDE_DIR ${SFML_REPO}/extlibs/headers/AL)
set(FREETYPE_LIBRARY ${SFML_REPO}/extlibs/libs-android/${ANDROID_ABI}/libfreetype.a)
set(VORBISENC_LIBRARY ${SFML_REPO}/extlibs/libs-android/${ANDROID_ABI}/libvorbisenc.a)
set(VORBISFILE_LIBRARY ${SFML_REPO}/extlibs/libs-android/${ANDROID_ABI}/libvorbisfile.a)
set(VORBIS_LIBRARY ${SFML_REPO}/extlibs/libs-android/${ANDROID_ABI}/libvorbis.a)
set(OGG_LIBRARY ${SFML_REPO}/extlibs/libs-android/${ANDROID_ABI}/libogg.a)
set(VORBIS_INCLUDE_DIR ${SFML_REPO}/extlibs/headers/vorbis)
set(OGG_INCLUDE_DIR ${SFML_REPO}/extlibs/headers/ogg)
set(FLAC_INCLUDE_DIR ${SFML_REPO}/extlibs/headers/flac)
set(FLAC_LIBRARY ${SFML_REPO}/extlibs/libs-android/${ANDROID_ABI}/libFLAC.a)
set(CMAKE_FIND_FRAMEWORK NEVER)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../extern/SFML ./sfml-build)


file(GLOB BATTLENETWORK_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../../BattleNetwork/*.cpp)

include_directories(native-lib ${CMAKE_CURRENT_SOURCE_DIR}/../../extern/includes)
include_directories(native-lib ${CMAKE_CURRENT_SOURCE_DIR}/../../BattleNetwork)

add_library( # Sets the name of the library.
        native-lib

        # Sets the library as a shared library.
        SHARED

        ${BATTLENETWORK_SRC}
        # Provides a relative path to your source file(s).
        src/main/cpp/native-lib.cpp)




# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.

        native-lib

        # Links the target library to the log library
        # included in the NDK.

        log
        android
        EGL
        GLESv1_CM
        
        -L${SFML_BUILD}/
        sfml-system
        sfml-window
        sfml-audio
        sfml-graphics
        sfml-network
        sfml-activity
        -Wl,--whole-archive sfml-main -Wl,--no-whole-archive)