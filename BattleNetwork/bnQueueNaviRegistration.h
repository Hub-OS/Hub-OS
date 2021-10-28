/*! \file bnQueueNaviRegistration.h */

/*! \brief This function hooks into the loading phase and loads extra content
 * 
 * This will be replaced by a script parser to load extra content without 
 * needing to recompile... */

#pragma once

// TODO: mac os < 10.5 file system support...
#ifndef __APPLE__
#include <filesystem>
#endif 

#include "bnPlayerPackageManager.h"
#include "bnResourceHandle.h"
#include "bnTextureResourceManager.h"
#include "stx/zip_utils.h"

#ifdef BN_MOD_SUPPORT
#include "bindings/bnScriptedPlayer.h"
#endif

static inline void QueuNaviRegistration(PlayerPackageManager& packageManager) {
  ResourceHandle handle;

#if defined(BN_MOD_SUPPORT) && !defined(__APPLE__)
  // Script resource manager load scripts from designated folder "resources/mods/players"
  std::string path_str = "resources/mods/players";
  for (const auto& entry : std::filesystem::directory_iterator(path_str)) {
    auto full_path = std::filesystem::absolute(entry).string();

    if (full_path.find(".zip") == std::string::npos) {
      try {
        if (auto res = packageManager.LoadPackageFromDisk<ScriptedPlayer>(full_path); res.is_error()) {
          Logger::Logf("%s", res.error_cstr());
        }
      }
      catch (std::runtime_error& e) {
        Logger::Logf("Player package unknown error: %s", e.what());
      }
    }
  }
#endif
}