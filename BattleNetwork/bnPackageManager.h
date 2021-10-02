#pragma once

#include "bnLogger.h"
#include "bnResourceHandle.h"
#include "bnScriptResourceManager.h"
#include "stx/result.h"
#include "stx/tuple.h"
#include "stx/zip_utils.h"
#include "stx/crypto_utils.h"
#include <vector>
#include <map>
#include <string>
#include <functional>
#include <atomic>

template<typename MetaClass>
class PackageManager {
    public:
    template<typename DataType>
    struct Meta {
        std::function<void()> loadClass;
        std::string filepath;
        std::string packageId;
        std::string fingerprint;

        DataType* data{nullptr};

        template<typename SuperDataType, typename... Args> MetaClass& SetClass(Args&&...);

        /*Implementation defined Post Parse-ops */
        virtual void OnMetaParsed() {} // After meta data is parsed
        virtual void PreGetData() {} // Before returning the allocated object

        MetaClass& SetPackageID(const std::string& id) {
          packageId = id;
          return *(static_cast<MetaClass*>(this));
        }

        MetaClass& SetFilePath(const std::string& filepath) {
          this->filepath = filepath;
          return *(static_cast<MetaClass*>(this));
        }

        MetaClass& SetPackageFingerprint(const std::string& hash) {
          this->fingerprint = hash;
          return *(static_cast<MetaClass*>(this));
        }

        const std::string& GetPackageID() const {
          return packageId;
        }

        const std::string& GetFilePath() const {
          return filepath;
        }

        const std::string& GetPackageFingerprint() const {
          return fingerprint;
        }

        DataType* GetData() {
          this->PreGetData();

          DataType* out = data;
          data = nullptr;
            
          loadClass();

          return out;
        }

        ~Meta() {
          delete data;
        }
    };

    //
    // member methods
    //
    PackageManager() {}
    PackageManager(const PackageManager&) = delete;
    PackageManager(PackageManager&&) = delete;
    virtual ~PackageManager();

    stx::result_t<bool> Commit(MetaClass* entry);

    template<class SuperDataType, typename... Args>
    MetaClass* CreatePackage(Args&&... args) {
      MetaClass* package = new MetaClass();
      package->template SetClass<SuperDataType>(std::forward<decltype(args)>(args)...);
      return package;
    }

    MetaClass& FindPackageByID(const std::string& id);
    std::string FilepathToPackageID(const std::string& id);
    const MetaClass& FindPackageByID(const std::string& id) const;
    const std::string FilepathToPackageID(const std::string& id) const;
    stx::result_t<std::string> RemovePackageByID(const std::string& id);

    bool HasPackage(const std::string& id) const;
    const std::string FirstValidPackage() const;
    const std::string GetPackageBefore(const std::string& id) const;
    const std::string GetPackageAfter(const std::string& id) const;
    stx::result_t<std::string> GetPackageFilePath(const std::string& id) const;

    /**
    * @brief Used at startup, loads every package queued by commits
    * @param progress atomic thread safe counter when loading resources
    */
    void LoadAllPackages(std::atomic<int>& progress);

    template<typename ScriptedDataType>
    stx::result_t<bool> LoadPackageFromDisk(const std::string& path);

    template<typename ScriptedDataType>
    stx::result_t<bool> LoadPackageFromZip(const std::string& path);

    /**
    * @brief Get the size of the package list
    * @return const unsigned size
    */
    const unsigned Size() const;

    private:
    std::map<std::string, MetaClass*> packages;
    std::map<std::string, std::string> filepathToPackageId;
    std::map<std::string, std::string> zipFilepathToPackageId;
};

template<typename MetaClass>
void PackageManager<MetaClass>::LoadAllPackages(std::atomic<int>& progress)
{
  for (auto& [key, entry] : packages) {
    entry->OnMetaParsed();

    Logger::Logf("Loaded package: %s", entry->packageId.c_str());

    progress++;
  }
}

template<typename MetaClass>
template<typename ScriptedDataType>
stx::result_t<bool> PackageManager<MetaClass>::LoadPackageFromDisk(const std::string& path)
{
#if defined(BN_MOD_SUPPORT) && !defined(__APPLE__)
  ResourceHandle handle;

  auto modpath = std::filesystem::absolute(path);
  modpath.make_preferred();

  std::string packageName = modpath.filename().generic_string();

  auto& res = handle.Scripts().LoadScript( modpath );

  if (res.result.valid()) {
    sol::state& state = *res.state;

    auto packageClass = this->CreatePackage<ScriptedDataType>(std::ref(state));

    // run script includes
    state["do_includes"]();

    // run script on meta info object
    state["package_init"](packageClass);

    packageClass->OnMetaParsed();
   
    std::string file_path = modpath.generic_string();
    packageClass->SetFilePath(file_path);

    auto zip_result = stx::zip(file_path, file_path + ".zip");
    if (zip_result.is_error()) {
      delete packageClass;
      std::string msg = std::string("Failed to install package ") + packageName + ". Reason: " + zip_result.error_cstr();
      return stx::error<bool>(msg);
    }

    auto md5_result = stx::generate_md5_from_file(file_path + ".zip");
    if (md5_result.is_error()) {
      delete packageClass;
      std::string msg = std::string("Failed to install package ") + packageName + ". Reason: " + md5_result.error_cstr();
      return stx::error<bool>(msg);
    }
    else {
      packageClass->SetPackageFingerprint(md5_result.value());
    }
    
    if (auto commit_result = this->Commit(packageClass); commit_result.is_error()) {
      delete packageClass;
      std::string msg = std::string("Failed to install package ") + packageName + ". Reason: " + commit_result.error_cstr();
      return stx::error<bool>(msg);
    }
  }
  else {
    sol::error sol_error = res.result;
    std::string msg = std::string("Failed to load package ") + packageName + ". Reason: " + sol_error.what();
    return stx::error<bool>(msg);
  }
#endif

  return stx::ok();
}

/**

TODO: Take ScriptedDataType out. Package manager should be used for scripted or on-disk resources by default now...

**/
template<typename MetaClass>
template<typename ScriptedDataType>
stx::result_t<bool> PackageManager<MetaClass>::LoadPackageFromZip(const std::string& path)
{
#if defined(BN_MOD_SUPPORT) && !defined(__APPLE__)
  auto absolute = std::filesystem::absolute(path);
  absolute = absolute.make_preferred();
  auto file = absolute.filename();
  std::string file_str = file.generic_string();
  size_t pos = file_str.find(".zip", 0);

  if (pos == std::string::npos) stx::error<bool>("Invalid zip file");

  file_str = file_str.substr(0, pos);

  absolute = absolute.remove_filename();
  std::string extracted_path = (absolute / std::filesystem::path(file_str)).generic_string();

  auto result = stx::unzip(path, extracted_path);

  if (result.value()) {
    return this->LoadPackageFromDisk<ScriptedDataType>(extracted_path);
  }

  return result;
#elif
  return stx::error<bool>("std::filesystem not supported");
#endif
}

/**
 * @brief Sets the deferred type constructor for T
 *
 * @return Meta& object for chaining
 */
template<typename MetaClass>
template<typename DataType>
template<typename SuperDataType, typename... Args>
inline MetaClass& PackageManager<MetaClass>::Meta<DataType>::SetClass(Args&&... args)
{
  loadClass = [this, tuple_args = std::make_tuple(std::forward<decltype(args)>(args)...)]() mutable {
    using TupleArgsType = decltype(tuple_args);

    data = (DataType*)stx::make_ptr_from_tuple<SuperDataType, TupleArgsType>(std::forward<TupleArgsType>(tuple_args));
  };

  return *static_cast<MetaClass*>(this);
}

template<typename MetaClass>
inline MetaClass& PackageManager<MetaClass>::FindPackageByID(const std::string& id)
{
  auto iter = packages.find(id);

  if (iter == packages.end()) {
    std::string error = "Package manager could not find package ID " + id;
    throw std::runtime_error(error);
  }

  return *(iter->second);
}

template<typename MetaClass>
inline std::string PackageManager<MetaClass>::FilepathToPackageID(const std::string& file_path) {
  auto absolute = std::filesystem::absolute(file_path);
  absolute = absolute.make_preferred();

  std::string full_path = absolute.generic_string();
  auto iter = filepathToPackageId.find(full_path);

  if (iter == filepathToPackageId.end()) {

    auto iter = zipFilepathToPackageId.find(full_path);
    if (iter == zipFilepathToPackageId.end()) {
      Logger::Logf("Package manager could not find package with filepath %s. (also tested %s)", file_path.c_str(), full_path.c_str());
      return "";
    }
    else {
      return iter->second;
    }
  }

  return iter->second;
}

template<typename MetaClass>
inline const MetaClass& PackageManager<MetaClass>::FindPackageByID(const std::string& id) const {
  auto iter = packages.find(id);

  if (iter == packages.end()) {
    std::string error = "Package manager could not find package ID " + id;
    throw std::runtime_error(error);
  }

  return *(iter->second);
}

template<typename MetaClass>
inline const std::string PackageManager<MetaClass>::FilepathToPackageID(const std::string& file_path) const {
  auto absolute = std::filesystem::absolute(file_path);
  absolute = absolute.make_preferred();
  std::string full_path = absolute.string();
  auto iter = filepathToPackageId.find(full_path);

  if (iter == filepathToPackageId.end()) {

    auto iter = zipFilepathToPackageId.find(full_path);
    if (iter == zipFilepathToPackageId.end()) {
      Logger::Logf("Package manager could not find package with filepath %s. (also tested %s)", file_path.c_str(), full_path.c_str());
    }

    return "";
  }

  return iter->second;
}

template<typename MetaClass>
PackageManager<MetaClass>::~PackageManager<MetaClass>() {
  for (auto& [_, p] : packages) {
    delete p;
  }

  packages.clear();
}

template<typename MetaClass>
stx::result_t<bool> PackageManager<MetaClass>::Commit(MetaClass* package)
{
  std::string packageId = package->GetPackageID();

  if (!package) {
    return stx::error<bool>(std::string("package object was nullptr"));
  }

  if (packageId.empty()) {
    return stx::error<bool>(std::string("package ID was not set"));
  }

  if (packages.find(packageId) != packages.end()) {
    return stx::error<bool>(std::string("There is already a package in the package manager with id of ") + packageId);
  }

  try {
    package->loadClass();
  }
  catch (std::runtime_error& e) {
    return stx::error<bool>(e.what());
  }

  packages.insert(std::make_pair(packageId, package));
  filepathToPackageId.insert(std::make_pair(package->GetFilePath(), packageId));
  zipFilepathToPackageId.insert(std::make_pair(package->GetFilePath() + ".zip", packageId));
  return stx::ok();
}

template<typename MetaClass>
stx::result_t<std::string> PackageManager<MetaClass>::RemovePackageByID(const std::string& id)
{
  if (auto iter = packages.find(id); iter != packages.end()) {
    packages.erase(iter);

    std::string path = iter->second->filepath;
    filepathToPackageId.erase(path);
    zipFilepathToPackageId.erase(path + ".zip");
    return stx::ok(path);
  }

  return stx::error<std::string>("No package with that ID");
}

template<typename MetaClass>
bool PackageManager<MetaClass>::HasPackage(const std::string& id) const
{
  return packages.find(id) != packages.end();
}

template<typename MetaClass>
const std::string PackageManager<MetaClass>::FirstValidPackage() const
{
  if (packages.empty()) {
    return std::string();
  }

  return packages.begin()->first;
}

template<typename MetaClass>
const std::string PackageManager<MetaClass>::GetPackageBefore(const std::string& id) const
{
  std::string previous_key;

  for (auto iter = packages.begin(); iter != packages.end(); iter = std::next(iter)) {
    std::string key = iter->first;

    if (key == id) {
      if (previous_key.empty()) {
        previous_key = packages.rbegin()->first;
      }

      break;
    }

    previous_key = key;
  }

  return previous_key;
}

template<typename MetaClass>
const std::string PackageManager<MetaClass>::GetPackageAfter(const std::string& id) const
{
  std::string previous_key;

  for (auto iter = packages.rbegin(); iter != packages.rend(); iter = std::next(iter)) {
    std::string key = iter->first;

    if (key == id) {
      if (previous_key.empty()) {
        previous_key = packages.begin()->first;
      }

      break;
    }

    previous_key = key;
  }

  return previous_key;
}

template<typename MetaClass>
stx::result_t<std::string> PackageManager<MetaClass>::GetPackageFilePath(const std::string& id) const
{
  if (this->HasPackage(id)) {
    const MetaClass& meta = this->FindPackageByID(id);
    return stx::ok(meta.GetFilePath());
  }

  return stx::error<std::string>("No package found");
}

template<typename MetaClass>
const unsigned PackageManager<MetaClass>::Size() const
{
  return (unsigned)packages.size();
}