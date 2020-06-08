cmake_minimum_required(VERSION 3.12)
    
project(BattleNetwork-Engine)
set (CMAKE_CXX_STANDARD 17)

execute_process(COMMAND git submodule update --init -- extern/Swoosh
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

execute_process(COMMAND git submodule update --init -- extern/lua
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

execute_process(COMMAND git submodule update --init -- extern/SFML
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

execute_process(COMMAND git submodule update --init -- extern/WebAPI
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake)

include_directories(extern/Swoosh/src)
include_directories(extern/lua)

add_subdirectory(extern/SFML)
add_subdirectory(extern/WebApi)

file(GLOB_RECURSE bnFiles CONFIGURE_DEPENDS
        "BattleNetwork/*.h"
        "BattleNetwork/*.cpp"
        "BattleNetwork/mmbn.ico.c"
        )

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/AddFiles.cmake)
add_executable(BattleNetwork BattleNetwork/main.cpp ${bnFiles} ${addBNFiles})

source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${bnFiles})
source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${addBNFiles})

target_link_libraries(BattleNetwork sfml-graphics sfml-audio sfml-network sfml-system sfml-window)
target_link_libraries(BattleNetwork WebAPI-client)

set_target_properties( BattleNetwork
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/$<CONFIG>"
)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Compiler.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/PostBuild.cmake)