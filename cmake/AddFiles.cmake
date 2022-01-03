# Should populate addBNFiles with additional files for the BattleNetwork target, depending on
# configuration, platform, etc.
# This file is included after project and before target declarations in parent CMakeLists.

set(addBNFiles "")

if(MSVC)
    
    # needed for the application icon
    list(APPEND addBNFiles "${CMAKE_CURRENT_SOURCE_DIR}/proj/msvc/BattleNetwork.rc")

endif()
