execute_process(COMMAND git rev-parse --short HEAD
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                OUTPUT_VARIABLE GIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE
                RESULT_VARIABLE GIT_RESULT)

if(NOT GIT_RESULT EQUAL "0")
    message(FATAL_ERROR "git command line program not found or .git file missing in working directory")
endif()

message(STATUS "Current branch is ${GIT_HASH}")
add_definitions(-DONB_BUILD_HASH="${GIT_HASH}")