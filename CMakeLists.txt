cmake_minimum_required(VERSION 3.1)
set (CMAKE_CXX_STANDARD 14)

project(BattleNetwork-Engine)

execute_process(COMMAND git submodule update --init -- extern/Swoosh
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(extern/Swoosh/src)


file(GLOB bnFiles
        "BattleNetwork/bn*.h"
        "BattleNetwork/bn*.cpp"
        "BattleNetwork/mmbn.ico.c"
        "BattleNetwork/resource.h"
        "BattleNetwork/Segues/*.h"
        )

find_package(SFML 2.5 COMPONENTS graphics audio network system window)

if(SFML_FOUND)
    add_executable(BattleNetwork BattleNetwork/main.cpp ${bnFiles})
    target_link_libraries(BattleNetwork sfml-graphics sfml-audio sfml-network sfml-system sfml-window)
else()
    execute_process(COMMAND git submodule update --init -- extern/includes/SFML
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
                    
    add_subdirectory(extern/SFML)

    add_executable(BattleNetwork BattleNetwork/main.cpp ${bnFiles})
    target_link_libraries(BattleNetwork sfml-graphics sfml-audio sfml-network sfml-system sfml-window)
endif()
