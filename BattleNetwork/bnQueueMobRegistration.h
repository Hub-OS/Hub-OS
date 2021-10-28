
/*! \file bnQueueMobRegistration.h*/

/*! \brief This hooks into the loading phase and loads extra content
 * 
 * This info adds mobs to the Mob Select screen to select 
 * Descriptions should be short as user cannot continue long
 * paragraphs */

#pragma once

// TODO: mac os < 10.5 file system support...
#ifndef __APPLE__
#include <filesystem>
#endif 

#include "bnMobPackageManager.h"

#ifdef BN_MOD_SUPPORT
#include "bindings/bnScriptedMob.h"
#endif

/***********************************************************************
************    Register your custom mobs here    *********************
************************************************************************/

static inline void QueueMobRegistration(MobPackageManager& packageManager) {
#if defined(BN_MOD_SUPPORT) && !defined(__APPLE__)
  // Script resource manager load scripts from designated folder "resources/mods/players"
  std::string path_str = "resources/mods/enemies";
  for (const auto& entry : std::filesystem::directory_iterator(path_str)) {
    auto full_path = std::filesystem::absolute(entry).string();

    if (full_path.find(".zip") == std::string::npos) {
      // TODO: check paths first to get rid of try-catches
      try {
        if (auto res = packageManager.LoadPackageFromDisk<ScriptedMob>(full_path); res.is_error()) {
          Logger::Logf("%s", res.error_cstr());
        }
      }
      catch (std::runtime_error& e) {
        Logger::Logf("Mob package unknown error: %s", e.what());
      }
    }
  }
#endif
}