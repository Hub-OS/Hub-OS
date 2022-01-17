/*! \file bnQueueModRegistration.h */
/*! \brief This function hooks into the loading phase and loads extra content */

#pragma once
#include <vector>
#include <functional>
#include <filesystem>

template<typename PackageManagerT, typename ScriptedResourceT>
static inline stx::result_t<bool> InstallMod(PackageManagerT& packageManager, const std::filesystem::path& fullModPath) {
  auto result = packageManager.template LoadPackageFromDisk<ScriptedResourceT>(fullModPath);

  if (!result.is_error()) {
    return stx::ok();
  }

  return stx::error<bool>(result.error_cstr());
}

template<typename PackageManagerT, typename ScriptedResourceT>
static inline void QueueModRegistration(PackageManagerT& packageManager, const const std::filesystem::path& modPath, const char* modCategory) {
#if defined(BN_MOD_SUPPORT)
  std::map<std::filesystem::path, bool> ignoreList;
  std::vector<std::filesystem::path> zipList;

  std::filesystem::create_directories(modPath);

  for (const auto& entry : std::filesystem::directory_iterator(modPath)) {
      std::filesystem::path full_path = std::filesystem::absolute(entry);

    if (ignoreList.find(full_path) != ignoreList.end())
      continue;

    // ignore readmes and text files
    if (full_path.extension() == ".txt" || full_path.extension() == ".md")
      continue;

    try {
      if (full_path.extension() != ".zip") {
        ignoreList[full_path] = true;
        std::filesystem::path zip_full_path = full_path;
        zip_full_path.concat(".zip");
        ignoreList[zip_full_path] = true;

        if (auto res = InstallMod<PackageManagerT, ScriptedResourceT>(packageManager, full_path); res.is_error()) {
          throw std::runtime_error(res.error_cstr());
        }
      }
      else {
        zipList.push_back(full_path);
      }
    }
    catch (std::runtime_error& e) {
      Logger::Logf(LogLevel::critical, "[%s] extracted package error: %s", modCategory, e.what());
    }
  }

  for (const auto& path : zipList) {
    // we have already loaded this mod, skip it
    if (ignoreList.find(path) != ignoreList.end())
      continue;

    try {
      if (auto res = packageManager.template LoadPackageFromZip<ScriptedResourceT>(path); res.is_error()) {
        throw std::runtime_error(res.error_cstr());
      }
    }
    catch (std::runtime_error& e) {
      Logger::Logf(LogLevel::critical, "[%s] .zip package error %s", modCategory, e.what());
    }
  }
#endif
}
