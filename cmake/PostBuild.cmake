# Should define post-build steps (e.g. resource, dll copying) for the BattleNetwork target
# This file is included at the end of the parent CMakeLists.txt

if(WIN32)

    ADD_CUSTOM_COMMAND(
        TARGET BattleNetwork
        POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        ARGS -E copy_if_different 
        
        "${PROJECT_SOURCE_DIR}/extern/lib/win32/openal32.dll"
        
        "$<TARGET_FILE_DIR:BattleNetwork>/"
        
        COMMENT "Copying openal32.dll\n"
    )

endif()

if(WIN32 OR UNIX)

    ADD_CUSTOM_COMMAND(
        TARGET BattleNetwork
        POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        ARGS -E copy_directory 
        
        "${PROJECT_SOURCE_DIR}/BattleNetwork/resources"
        
        "$<TARGET_FILE_DIR:BattleNetwork>/resources"
        
        COMMENT "Copying resources\n"
    )

    ADD_CUSTOM_COMMAND(
        TARGET BattleNetwork
        POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        ARGS -E copy_if_different 
        
        "${PROJECT_SOURCE_DIR}/BattleNetwork/config.ini"
        
        "$<TARGET_FILE_DIR:BattleNetwork>/"
        
        COMMENT "Copying config.ini\n"
    )

endif()

if(BN_USE_SHARED_LIBS)

    ADD_CUSTOM_COMMAND(
        TARGET BattleNetwork
        POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        ARGS -E copy_directory
        
        "$<TARGET_FILE_DIR:WebAPI-client>"
        "$<TARGET_FILE_DIR:sfml-graphics>"
        "$<TARGET_FILE_DIR:sfml-audio>"
        "$<TARGET_FILE_DIR:sfml-network>"
        "$<TARGET_FILE_DIR:sfml-system>"
        "$<TARGET_FILE_DIR:sfml-window>"
        
        "$<TARGET_FILE_DIR:BattleNetwork>"
        
        COMMENT "Copying shared libraries\n"
    )

endif()