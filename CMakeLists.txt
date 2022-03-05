cmake_minimum_required(VERSION 3.12)

project(BattleNetwork-Engine)
set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(FindLua)

execute_process(COMMAND git submodule update --init -- extern/Swoosh
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

execute_process(COMMAND git submodule update --init -- extern/SFML
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake)

find_package(Poco REQUIRED Foundation Net)
find_package(Threads REQUIRED)
find_package(Lua REQUIRED)
find_package(FluidSynth REQUIRED)
# find_package(sol2 REQUIRED) # commented out until sol2 vcpkg is up to date

include_directories(extern/Swoosh/src)
include_directories(BattleNetwork/sfMidi/include)
include_directories(BattleNetwork/sfMidi/src)
include_directories(BattleNetwork/sol/include) # temporary sol2 bugfix
add_subdirectory(extern/SFML)

file(GLOB_RECURSE bnFiles CONFIGURE_DEPENDS
        "BattleNetwork/*.h"
        "BattleNetwork/*.cpp"
	"BattleNetwork/netplay/*.h"
	"BattleNetwork/netplay/*.cpp"
	"BattleNetwork/overworld/*.h"
	"BattleNetwork/overworld/*.cpp"
	"BattleNetwork/widgets/*.h"
	"BattleNetwork/widgets/*.cpp"
        "BattleNetwork/bindings/*.h"
        "BattleNetwork/bindings/*.cpp"
        "BattleNetwork/stx/*.h"
        "BattleNetwork/stx/*.cpp"
        "BattleNetwork/zip/*.h"
        "BattleNetwork/zip/*.c"
        "BattleNetwork/crypto/*.h"
        "BattleNetwork/sago/*.h"
        "BattleNetwork/sago/*.cpp"
        "BattleNetwork/mmbn.ico.c"
        )

list(FILTER bnFiles EXCLUDE REGEX "BattleNetwork/neurolink/navi_personality_draft_5_memories.cpp")

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/AddFiles.cmake)

# ScriptResourceManager has sol2 templates that generate massive number of obj sections...
# further reading: https://docs.microsoft.com/en-us/cpp/build/reference/bigobj-increase-number-of-sections-in-dot-obj-file?view=msvc-160
if(MSVC)
	add_compile_options(/bigobj)
endif()

add_executable(BattleNetwork BattleNetwork/main.cpp ${bnFiles} ${addBNFiles})
add_compile_definitions(NOMINMAX)

# This tells BN engine to define a ScriptResourceMananger and
# load mods during the boot step
# WARNING: sol2 bindings increase compile times by 2-3x! 
#          Uncomment this only if you're working with scripting!
add_compile_definitions(BN_MOD_SUPPORT)

source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${bnFiles})
source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${addBNFiles})

target_compile_definitions(BattleNetwork PRIVATE SOL_ALL_SAFETIES_ON)

target_include_directories(BattleNetwork PRIVATE ${LUA_INCLUDE_DIR})
target_link_libraries(BattleNetwork sfml-graphics sfml-audio sfml-network sfml-system sfml-window)
target_link_libraries(BattleNetwork ${FLUIDSYNTH_LIBRARIES})
target_link_libraries(BattleNetwork Poco::Net Poco::Foundation)
target_link_libraries(BattleNetwork Threads::Threads)
target_link_libraries(BattleNetwork ${LUA_LIBRARIES})
# target_link_libraries(BattleNetwork sol2::sol2)

set_target_properties(BattleNetwork
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/$<CONFIG>"
)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Compiler.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/PostBuild.cmake)
