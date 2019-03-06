cmake_minimum_required(VERSION 3.1)
set (CMAKE_CXX_STANDARD 14)

project(BattleNetwork-Engine)

## If you want to link SFML statically
# set(SFML_STATIC_LIBRARIES TRUE)

## In most cases better set in the CMake cache
# set(SFML_DIR "<sfml root prefix>/lib/cmake/SFML")

find_package(SFML 2.5 COMPONENTS graphics audio network system window REQUIRED)

file(GLOB bnFiles
        "BattleNetwork/bn*.h"
        "BattleNetwork/bn*.cpp"
        "BattleNetwork/AlphaBossSpawnPolicy.h"
        "BattleNetwork/mmbn.ico.c"
        "BattleNetwork/resource.h"
        "BattleNetwork/Segues/*.h"
        )

include_directories(extern/includes)
add_executable(BattleNetwork BattleNetwork/main.cpp ${bnFiles})
target_link_libraries(BattleNetwork sfml-graphics sfml-audio sfml-network sfml-system sfml-window)
