# Should contain compiler-dependent configuration for the OpenNetBattle project
# This file is included after project and after target declarations in parent CMakeLists.

if(MSVC)
    set_property(TARGET BattleNetwork 
                PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/$<CONFIG>")
	
    add_definitions(/MP)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" "/MP")
    MESSAGE(STATUS "Added parallel build arguments to CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
endif()

if(NOT MSVC)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-value -Wno-inconsistent-missing-override -Wno-switch")

endif()
