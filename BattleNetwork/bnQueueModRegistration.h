/*! \file bnQueueModRegistration.h */
/*! \brief This function hooks into the loading phase and loads extra content */

#pragma once
 // TODO: mac os < 10.5 file system support...
#ifndef __APPLE__
#include <filesystem>
#endif 

template<typename PackageManagerT, typename ScriptedResourceT>
static inline void QueueModRegistration(PackageManagerT& packageManager, const char* modPath, const char* modCategory) {
#if defined(BN_MOD_SUPPORT) && !defined(__APPLE__)
  std::map<std::string, bool> ignoreList;
  std::vector<std::string> zipList;

  for (const auto& entry : std::filesystem::directory_iterator(modPath)) {
    auto full_path = std::filesystem::absolute(entry).string();

    if (ignoreList.find(full_path) != ignoreList.end())
      continue;

    try {
      if (size_t pos = full_path.find(".zip"); pos == std::string::npos) {
        if (auto res = packageManager.LoadPackageFromDisk<ScriptedResourceT>(full_path); res.is_error()) {
          throw std::runtime_error(res.error_cstr());
          continue;
        }

        ignoreList[full_path] = true;
        ignoreList[full_path+".zip"] = true;
      }
      else {
        zipList.push_back(full_path);
      }
    }
    catch (std::runtime_error& e) {
      Logger::Logf("[%s] extracted package error: %s", modCategory, e.what());
    }
  }

  for (const auto& path : zipList) {
    // we have already loaded this mod, skip it
    if (ignoreList.find(path) != ignoreList.end())
      continue;

    try {
      if (auto res = packageManager.LoadPackageFromZip<ScriptedResourceT>(path); res.is_error()) {
        throw std::runtime_error(res.error_cstr());
      }
    }
    catch (std::runtime_error& e) {
      Logger::Logf("[%s] .zip package error %s", modCategory, e.what());
    }
  }
#endif
}