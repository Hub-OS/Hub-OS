#pragma once

#include "stx/result.h"
#include <vector>
#include <map>
#include <string>
#include <functional>

template<typename MetaClass>
class PackageManager {
    public:
    template<typename DataType>
    struct Meta {
        std::function<void()> loadClass;
        std::string filepath;
        std::string packageId;
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

        const std::string& GetPackageID() const {
          return packageId;
        }

        const std::string& GetFilePath() const {
          return filepath;
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

    virtual ~PackageManager();

    stx::result_t<bool> Commit(MetaClass* entry);

    template<class SuperDataType, typename... Args>
    MetaClass* CreatePackage(Args&&... args) {
        MetaClass* package = new MetaClass();
        package->SetClass<SuperDataType>(std::forward<decltype(args)>(args)...);
        return package;
    }

    MetaClass& FindPackageByID(const std::string& id);
    const MetaClass& FindPackageByID(const std::string& id) const;
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
};

template<typename MetaClass>
void PackageManager<MetaClass>::LoadAllPackages(std::atomic<int>& progress)
{
  for (auto& [key, entry] : packages) {
    entry->loadClass();
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

  const auto& modpath = std::filesystem::absolute(path);
  auto entrypath = modpath / "entry.lua";
  std::string packageName = modpath.filename().string();
  auto& res = handle.Scripts().LoadScript(entrypath.string());

  if (res.result.valid()) {
    sol::state& state = *res.state;

    // Sets the predefined _modpath variable
    state["_modpath"] = modpath.string() + "/";

    auto packageClass = this->CreatePackage<ScriptedDataType>(std::ref(state));

    // run script on meta info object
    state["package_init"](packageClass);

    packageClass->OnMetaParsed();

    packageClass->SetFilePath(modpath.string());
    
    if (auto result = this->Commit(packageClass); result.is_error()) {
      std::string msg = std::string("Failed to install package ") + packageName + ". Reason: " + result.error_cstr();
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
stx::result_t<bool> PackageManager<typename MetaClass>::LoadPackageFromZip(const std::string& path)
{
#if defined(BN_MOD_SUPPORT) && !defined(__APPLE__)
  auto absolute = std::filesystem::absolute(path);
  auto file = absolute.filename();
  std::string file_str = file.string();
  size_t pos = file_str.find(".zip", 0);

  if (pos == std::string::npos) stx::error<bool>("Invalid zip file");

  file_str = file_str.substr(0, pos);

  absolute = absolute.remove_filename();
  std::string extracted_path = (absolute / std::filesystem::path(file_str)).string();

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
inline MetaClass& PackageManager<typename MetaClass>::Meta<DataType>::SetClass(Args&&... args)
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
inline const MetaClass& PackageManager<MetaClass>::FindPackageByID(const std::string& id) const {
  auto iter = packages.find(id);

  if (iter == packages.end()) {
    std::string error = "Package manager could not find package ID " + id;
    throw std::runtime_error(error);
  }

  return *(iter->second);
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
  if (package && !packageId.empty()) {
    if (packages.find(packageId) != packages.end()) {
      return stx::error<bool>(std::string("There is already a package in the package manager with id of ") + packageId);
    }
  }
  else {
    return stx::error<bool>(std::string("package object was nullptr or package ID was not set"));
  }

  try {
    package->loadClass();
  }
  catch (std::runtime_error& e) {
    return stx::error<bool>(e.what());
  }

  packages.insert(std::make_pair(packageId, package));

  return stx::ok();
}

template<typename MetaClass>
stx::result_t<std::string> PackageManager<MetaClass>::RemovePackageByID(const std::string& id)
{
  if (auto iter = packages.find(id); iter != packages.end()) {
    std::string path = iter->second->filepath;
    packages.erase(iter);
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