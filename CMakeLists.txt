# SMPEG is a free MPEG1 video player library with sound support.
#
# Please see the file LICENSE.txt in the source's root directory.

set(SMPEG_VERSION 2.0.0)

cmake_minimum_required(VERSION 3.5)

project(SMPEG VERSION ${SMPEG_VERSION} LANGUAGES C CXX)

include(GNUInstallDirs)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "SunPro")
    add_compile_options(-erroff=E_EMPTY_TRANSLATION_UNIT)
    add_compile_options(-xldscope=hidden)
endif()

find_package(SDL2 REQUIRED)
if(WIN32)
    # -lmingw32: gcc adds it automatically.
    # -mwindows: doesn't make sense.
    # -lSDL2main: the dll doesn't need it, the test apps are console-only
    #  and specifically define SDL_MAIN_HANDLED.  it will be ignored.
    string(REGEX REPLACE "-mwindows" "" SDL2_LIBRARIES "${SDL2_LIBRARIES}")
    string(REGEX REPLACE "-lmingw32" "" SDL2_LIBRARIES "${SDL2_LIBRARIES}")
#   string(REGEX REPLACE "-lSDL2main" "" SDL2_LIBRARIES "${SDL2_LIBRARIES}")
    string(STRIP "${SDL2_LIBRARIES}" SDL2_LIBRARIES)
endif()

if(UNIX AND NOT (WIN32 OR APPLE OR HAIKU OR EMSCRIPTEN))
    find_library(LIBM_LIBRARY m)
    if(LIBM_LIBRARY)
        set(LIBM_LIBRARY m)
    endif()
endif()

if(UNIX AND NOT WIN32 AND NOT APPLE)  # (MingW and such might be UNIX _and_ WINDOWS!)
    find_library(PTHREAD_LIBRARY pthread)
    if(PTHREAD_LIBRARY)
        list(APPEND OPTIONAL_LIBRARY_LIBS ${PTHREAD_LIBRARY})
    endif()
endif()

if(LIBM_LIBRARY)
    list(APPEND ${LIBM_LIBRARY})
endif()

set(SMPEG_SRCS
    MPEG.cpp
    MPEGring.cpp
    MPEGlist.cpp
    MPEGstream.cpp
    MPEGsystem.cpp
    smpeg.cpp
    audio/MPEGaudio.cpp
    audio/bitwindow.cpp
    audio/filter.cpp
    audio/filter_2.cpp
    audio/hufftable.cpp
    audio/mpeglayer1.cpp
    audio/mpeglayer2.cpp
    audio/mpeglayer3.cpp
    audio/mpegtable.cpp
    audio/mpegtoraw.cpp
    video/MPEGvideo.cpp
    video/decoders.cpp
    video/decoders.h
    video/dither.h
    video/floatdct.cpp
    video/gdith.cpp
    video/jrevdct.cpp
    video/motionvec.cpp
    video/parseblock.cpp
    video/proto.h
    video/readfile.cpp
    video/util.cpp
    video/util.h
    video/video.cpp
    video/video.h
)

option(SMPEG_TIMESTAMP_SYNC "Enable system timestamp sync" FALSE)
if(SMPEG_TIMESTAMP_SYNC)
    add_definitions(-DUSE_TIMESTAMP_SYNC)
endif()

option(SMPEG_THREADED_SYSTEM "Enable system thread" FALSE)
if(SMPEG_THREADED_SYSTEM)
    add_definitions(-DUSE_SYSTEM_THREAD)
endif()

option(SMPEG_VIDEO_CALLBACK_THREAD "Enable video callback thread" TRUE)
if(NOT SMPEG_VIDEO_CALLBACK_THREAD)
    add_definitions(-DDISABLE_VIDEO_CALLBACK_THREAD)
endif()

option(SMPEG_THREADED_AUDIO "Enable threaded audio" TRUE)
if(SMPEG_THREADED_AUDIO)
    add_definitions(-DTHREADED_AUDIO)
endif()

option(SMPEG_ASSERTIONS "Enable consistency checks in decoding" FALSE)
if(SMPEG_ASSERTIONS)
    add_definitions(-DNDEBUG)
endif()

option(SMPEG_BUILD_STATIC "Build static library" TRUE)
if(SMPEG_BUILD_STATIC)
    add_library(smpeg2-static STATIC ${SMPEG_SRCS})
    add_library(SMPEG2::SMPEG2-static ALIAS smpeg2-static)
    set_target_properties(smpeg2-static PROPERTIES EXPORT_NAME SMPEG2-static)
    # Don't rename this on Windows, since DLLs will also produce an import
    #  library named "smpeg.lib" which would conflict; Unix tend to like the
    #  same library name with a different extension for static libs, but
    #  Windows can just have a separate name.
    if(NOT MSVC)
        set_target_properties(smpeg2-static PROPERTIES OUTPUT_NAME "smpeg2")
    endif()
    if(WINRT)
        # Ignore LNK4264 warnings; we don't author any WinRT components, just consume them, so we're okay in a static library.
		set_target_properties(smpeg2-static PROPERTIES VS_WINRT_COMPONENT True)
        set_target_properties(smpeg2-static PROPERTIES STATIC_LIBRARY_FLAGS "/ignore:4264")
    endif()
    if(WIN32 OR WINRT OR OS2)
        # no dll exports from the static library
        target_compile_definitions(smpeg2-static PRIVATE "SMPEG_STATIC")
    endif()
    target_include_directories(smpeg2-static PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>" "$<BUILD_INTERFACE:${SDL2_INCLUDE_DIRS}>")
    target_include_directories(smpeg2-static PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/audio" "${CMAKE_CURRENT_SOURCE_DIR}/video")
    target_link_libraries(smpeg2-static PRIVATE "$<BUILD_INTERFACE:${SDL2_LIBRARIES}>" ${OPTIONAL_LIBRARY_LIBS} ${OTHER_LDFLAGS})
    set(SMPEG_LIB_TARGET SMPEG2::SMPEG2-static)
    list(APPEND SMPEG_INSTALL_TARGETS "smpeg2-static")
endif()

option(SMPEG_BUILD_SHARED "Build shared library" TRUE)
if(SMPEG_BUILD_SHARED)
    add_library(smpeg2 SHARED ${SMPEG_SRCS})
    add_library(SMPEG2::SMPEG2 ALIAS smpeg2)
    set_target_properties(smpeg2 PROPERTIES MACOSX_RPATH 1)
    set_target_properties(smpeg2 PROPERTIES VERSION ${SMPEG_VERSION})
    set_target_properties(smpeg2 PROPERTIES EXPORT_NAME SMPEG2)
    target_include_directories(smpeg2 PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>" "$<BUILD_INTERFACE:${SDL2_INCLUDE_DIRS}>")
    target_include_directories(smpeg2 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/audio" "${CMAKE_CURRENT_SOURCE_DIR}/video")
    target_link_libraries(smpeg2 PRIVATE "$<BUILD_INTERFACE:${SDL2_LIBRARIES}>" ${OPTIONAL_LIBRARY_LIBS} ${OTHER_LDFLAGS})
    set(SMPEG_LIB_TARGET SMPEG2::SMPEG2)
    list(APPEND SMPEG_INSTALL_TARGETS "smpeg2")
endif()

if(NOT SMPEG_BUILD_SHARED AND NOT SMPEG_BUILD_STATIC)
    message(FATAL "Both shared and static libraries are disabled!")
endif()

option(SMPEG_BUILD_TEST "Build plaympeg test program." TRUE)
mark_as_advanced(SMPEG_BUILD_TEST)
if(SMPEG_BUILD_TEST)
    add_executable(plaympeg plaympeg.c)
    if(TARGET SDL2::SDL2)
        target_link_libraries(plaympeg PRIVATE SDL2::SDL2)
    else()
        target_include_directories(plaympeg PRIVATE ${SDL2_INCLUDE_DIRS})
        target_link_libraries(plaympeg PRIVATE ${SDL2_LIBRARIES})
    endif()
    target_link_libraries(plaympeg PRIVATE ${SMPEG_LIB_TARGET} ${OTHER_LDFLAGS})

    list(APPEND SMPEG_INSTALL_TARGETS plaympeg)
    IF (WIN32 AND MSVC)
        SET_TARGET_PROPERTIES(plaympeg PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
        SET_TARGET_PROPERTIES(plaympeg PROPERTIES COMPILE_DEFINITIONS _CONSOLE)
    ENDIF ()
    IF (CMAKE_COMPILER_IS_MINGW)
        SET_TARGET_PROPERTIES(plaympeg PROPERTIES LINK_FLAGS "-mconsole")
    ENDIF ()
    if(NOT SMPEG_BUILD_SHARED)
        target_link_libraries(plaympeg PRIVATE ${SDL2_LIBRARIES} ${OPTIONAL_LIBRARY_LIBS} ${OTHER_LDFLAGS})
    endif()
endif()

option(SMPEG_DISABLE_INSTALL "Disable installing SMPEG" OFF)
if(NOT SMPEG_DISABLE_INSTALL)

    install(TARGETS ${SMPEG_INSTALL_TARGETS} EXPORT SMPEGExport
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(FILES smpeg.h MPEGframe.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    install(EXPORT SMPEGExport
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SMPEG2"
            FILE SMPEG2Config.cmake
            NAMESPACE SMPEG2::
    )

    if(NOT MSVC)
        configure_file(
            "smpeg2-config.in"
            "smpeg2.spec.in"
            @ONLY
        )

        install(
            FILES "${CMAKE_CURRENT_BINARY_DIR}/smpeg2-config"
            DESTINATION "${CMAKE_INSTALL_BINDIR}"
        )
    endif()
endif()

macro(message_bool_option _NAME _VALUE)
    if(${_VALUE})
        message(STATUS "  ${_NAME}: enabled")
    else()
        message(STATUS "  ${_NAME}: disabled")
    endif()
endmacro()

message(STATUS "SMPEG2 will build with the following options:")
message_bool_option("System timestamp sync" SMPEG_TIMESTAMP_SYNC)
message_bool_option("System thread" SMPEG_THREADED_SYSTEM)
message_bool_option("Video callback thread" SMPEG_VIDEO_CALLBACK_THREAD)
message_bool_option("Threaded audio" SMPEG_THREADED_AUDIO)
message_bool_option("Consistency checks in decoding" SMPEG_ASSERTIONS)
message_bool_option("Build static library" SMPEG_BUILD_STATIC)
message_bool_option("Build shared library" SMPEG_BUILD_SHARED)

# end of CMakeLists.txt ...
