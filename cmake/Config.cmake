# Should define configuration options (flags, paths, etc.) for the OpenNetBattle project.
# This file is included after project and before target declarations in parent CMakeLists.

option(ONB_USE_SHARED_LIBS "Whether to use shared or static libs" ON ) #used by BattleNetwork
set(BUILD_SHARED_LIBS ${ONB_USE_SHARED_LIBS} CACHE BOOL "Whether to use shared or static libs" FORCE) #used by SFML, WebAPI
