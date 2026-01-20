cmake_minimum_required(VERSION 3.15)
project(boing VERSION 1.0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_mixer REQUIRED)

# If SDL2 targets are not available, use variables
if(NOT TARGET SDL2::SDL2)
    add_library(SDL2::SDL2 INTERFACE IMPORTED)
    set_target_properties(SDL2::SDL2 PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${SDL2_LIBRARIES}"
    )
endif()

# Sources
set(SOURCES
    app.cpp
    appconsole.cpp
    appdata.cpp
    audiomanager.cpp
    ball.cpp
    bmfont.cpp
    configdata.cpp
    configscreen.cpp
    floor.cpp
    gamerunner.cpp
    graph.cpp
    item.cpp
    logger.cpp
    menu.cpp
    minput.cpp
    pang.cpp
    player.cpp
    scene.cpp
    select.cpp
    shoot.cpp
    sprite.cpp
    stage.cpp
    stageclear.cpp
    textoverlay.cpp
)

# Headers
set(HEADERS
    app.h
    appconsole.h
    appdata.h
    audiomanager.h
    ball.h
    bmfont.h
    configscreen.h
    floor.h
    gameobject.h
    gamerunner.h
    graph.h
    item.h
    logger.h
    menu.h
    minput.h
    pang.h
    player.h
    scene.h
    select.h
    shoot.h
    sprite.h
    stage.h
    stageclear.h
    textoverlay.h
    resource.h
)

if(WIN32)
    list(APPEND SOURCES pang.rc)
endif()

add_executable(boing ${SOURCES} ${HEADERS})

target_include_directories(boing PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(boing
    PRIVATE
    SDL2::SDL2
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_MIXER_LIBRARIES}
)

# Also handle cases where SDL2_image/mixer might have targets
if(TARGET SDL2_image::SDL2_image)
    target_link_libraries(boing PRIVATE SDL2_image::SDL2_image)
else()
    target_include_directories(boing PRIVATE ${SDL2_IMAGE_INCLUDE_DIRS})
endif()

if(TARGET SDL2_mixer::SDL2_mixer)
    target_link_libraries(boing PRIVATE SDL2_mixer::SDL2_mixer)
else()
    target_include_directories(boing PRIVATE ${SDL2_MIXER_INCLUDE_DIRS})
endif()

if(WIN32)
    target_link_libraries(boing PRIVATE winmm odbc32 odbccp32)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT boing)

    # Use WINDOWS_EXPORT_ALL_SYMBOLS if needed, but not for EXE
    # Set entry point if necessary, or let SDL2 handle WinMain
    # The vcxproj had EntryPointSymbol: WinMainCRTStartup
    set_target_properties(boing PROPERTIES WIN32_EXECUTABLE ON)
    set_target_properties(boing PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$(ProjectDir)/.."
    )
endif()
