cmake_minimum_required(VERSION 3.15)
project(boing VERSION 1.0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define _DEBUG for Debug builds and NDEBUG for Release builds
# This ensures compatibility across platforms (Windows MSVC uses _DEBUG, but not GCC/Clang by default)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(_DEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(NDEBUG)
endif()

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
    src/main.cpp
    src/core/animcontroller.cpp
    src/core/asepriteloader.cpp
    src/core/action.cpp
    src/core/app.cpp
    src/ui/appconsole.cpp
    src/core/appdata.cpp
    src/core/audiomanager.cpp
    src/core/eventmanager.cpp
    src/core/jsonparser.cpp
    src/core/motion.cpp
    src/core/oncehelper.cpp
    src/entities/ball.cpp
    src/ui/bmfont.cpp
    src/core/configdata.cpp
    src/ui/configscreen.cpp
    src/game/floor.cpp
    src/core/gamerunner.cpp
    src/core/gameevent.cpp
    src/core/graph.cpp
    src/game/item.cpp
    src/core/logger.cpp
    src/ui/menu.cpp
    src/ui/menutitle.cpp
    src/core/minput.cpp
    src/entities/player.cpp
    src/entities/playerdeadaction.cpp
    src/game/scene.cpp
    src/ui/select.cpp
    src/entities/shot.cpp
    src/entities/harpoonshot.cpp
    src/entities/gunshot.cpp
    src/core/sprite.cpp
    src/core/sprite2D.cpp
    src/core/spritesheet.cpp
    src/game/stage.cpp
    src/game/stageclear.cpp
    src/game/stageloader.cpp
    src/game/weapontype.cpp
    src/ui/textoverlay.cpp
)

# Headers
set(HEADERS
    src/main.h
    src/core/action.h
    src/core/animcontroller.h
    src/core/app.h
    src/core/asepriteloader.h
    src/ui/appconsole.h
    src/core/appdata.h
    src/core/audiomanager.h
    src/core/eventmanager.h
    src/core/gameevent.h
    src/core/jsonparser.h
    src/core/motion.h
    src/core/oncehelper.h
    src/entities/ball.h
    src/ui/bmfont.h
    src/ui/configscreen.h
    src/game/floor.h
    src/core/gameobject.h
    src/core/gamerunner.h
    src/core/graph.h
    src/game/item.h
    src/core/logger.h
    src/ui/menu.h
    src/ui/menutitle.h
    src/core/minput.h
    src/entities/player.h
    src/entities/playerdeadaction.h
    src/game/scene.h
    src/ui/select.h
    src/entities/shot.h
    src/entities/harpoonshot.h
    src/entities/gunshot.h
    src/core/sprite.h
    src/core/sprite2D.h
    src/core/spritesheet.h
    src/game/stage.h
    src/game/stageclear.h
    src/game/stageloader.h
    src/game/weapontype.h
    src/ui/textoverlay.h
    src/resource.h
)

if(WIN32)
    list(APPEND SOURCES src/pang.rc)
endif()

add_executable(boing ${SOURCES} ${HEADERS})

target_include_directories(boing PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game
    ${CMAKE_CURRENT_SOURCE_DIR}/src/entities
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
)

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
