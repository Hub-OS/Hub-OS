cmake_minimum_required(VERSION 3.12)

project(BattleNetwork-Engine)
set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

execute_process(COMMAND git submodule update --init -- extern/Swoosh
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

execute_process(COMMAND git submodule update --init -- extern/SFML
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

execute_process(COMMAND git submodule update --init -- extern/WebAPI
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_compile_definitions(NOMINMAX)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
add_compile_definitions(_WINSOCK_DEPRECATED_NO_WARNINGS)
add_compile_definitions(SOL_ALL_SAFETIES_ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake)
include(FindLua)

find_package(lua REQUIRED)
find_package(Poco REQUIRED Foundation Net)
find_package(sol2 CONFIG REQUIRED)

add_subdirectory(extern/SFML)
add_subdirectory(extern/WebAPI)

file(GLOB_RECURSE bnFiles CONFIGURE_DEPENDS
        "BattleNetwork/*.h"
        "BattleNetwork/*.cpp"
	"BattleNetwork/netplay/*.h"
	"BattleNetwork/netplay/*.cpp"
        "BattleNetwork/mmbn.ico.c"
        )

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/AddFiles.cmake)
add_executable(BattleNetwork BattleNetwork/main.cpp ${bnFiles} ${addBNFiles})

source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${bnFiles})
source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${addBNFiles})

target_include_directories(BattleNetwork PRIVATE extern/Swoosh/src)
target_include_directories(BattleNetwork PRIVATE ${LUA_INCLUDE_DIR})

target_link_libraries(BattleNetwork sfml-graphics sfml-audio sfml-network sfml-system sfml-window)
target_link_libraries(BattleNetwork WebAPI-client)
target_link_libraries(BattleNetwork Poco::Net)
target_link_libraries(BattleNetwork ${LUA_LIBRARIES})
target_link_libraries(BattleNetwork sol2::sol2)

set_target_properties(BattleNetwork
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/$<CONFIG>"
)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Compiler.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/PostBuild.cmake)
