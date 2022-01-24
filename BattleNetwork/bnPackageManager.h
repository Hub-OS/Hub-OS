#pragma once

#include "bnPackageAddress.h"
#include "bnLogger.h"
#include "bnResourceHandle.h"
#include "bnScriptResourceManager.h"
#include "bnSolHelpers.h"
#include "stx/string.h"
#include "stx/result.h"
#include "stx/tuple.h"
#include "stx/zip_utils.h"
#include "stx/crypto_utils.h"
#include <vector>
#include <map>
#include <string>
#include <functional>
#include <atomic>

// forward decl.
template<typename MetaClass>
class PackageManager;

template<typename PackageManager>
class PackagePartitioner {
  using PackageManager_t = PackageManager;
  using MetaClass_t = typename PackageManager::MetaClass_t;

  // Paritions are identified by their "namespace" ID
  std::map<std::string, PackageManager_t*> partitions;
public:
  ~PackagePartitioner() {
    for (auto& [_, p] : partitions) {
      delete p;
    }

    partitions.clear();
  }

  const bool HasNamespace(const std::string& ns) {
    return partitions.find(ns) != partitions.end();
  }

  const bool HasPackage(const PackageAddress& addr) {
    if (HasNamespace(addr.namespaceId)) {
      return GetPartition(addr.namespaceId).HasPackage(addr.packageId);
    }

    return false;
  }

  MetaClass_t& FindPackageByAddress(const PackageAddress& addr) {
    return GetPartition(addr.namespaceId).FindPackageByID(addr.packageId);
  }

  void CreateNamespace(const std::string& ns) {
    if (HasNamespace(ns)) return;
    partitions.insert(std::pair<std::string, PackageManager_t*>(ns, new PackageManager_t(ns)));
  }

  PackageManager_t& GetPartition(const std::string& ns) {
    return *partitions.find(ns)->second;
  }
};

template<typename MetaClass>
class PackageManager {
    public:
    using MetaClass_t = MetaClass;

    template<typename DataType>
    struct Meta {
        std::function<void()> loadClass;
        std::filesystem::path filepath;
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

        MetaClass& SetFilePath(const std::filesystem::path& filepath) {
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

        const std::filesystem::path& GetFilePath() const {
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
    PackageManager(const std::string& namespaceId) : namespaceId(namespaceId) {};
    PackageManager() = delete;
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
    string FilepathToPackageID(const std::filesystem::path& file_path);
    string FilepathToPackageAddress(const std::filesystem::path& file_path);
    const MetaClass& FindPackageByID(const std::string& id) const;
    const string FilepathToPackageID(const std::filesystem::path& file_path) const;
    const string FilepathToPackageAddress(const std::filesystem::path& file_path) const;
    stx::result_t<std::filesystem::path> RemovePackageByID(const std::string& id);

    bool HasPackage(const std::string& id) const;
    const std::string FirstValidPackage() const;
    const std::string GetPackageBefore(const std::string& id) const;
    const std::string GetPackageAfter(const std::string& id) const;
    stx::result_t<std::filesystem::path> GetPackageFilePath(const std::string& id) const;

    /**
    * @brief Used at startup, loads every package queued by commits
    * @param progress atomic thread safe counter when loading resources
    */
    void LoadAllPackages(std::atomic<int>& progress);

    template<typename ScriptedDataType>
    stx::result_t<std::string> LoadPackageFromDisk(const std::filesystem::path& path);

    template<typename ScriptedDataType>
    stx::result_t<std::string> LoadPackageFromZip(const std::filesystem::path& path);

    /**
    * @brief Get the size of the package list
    * @return const unsigned size
    */
    const unsigned Size() const;

    /**
    * @brief Removes packages, file2PackageId, and zipFile2PackageId hashes from memory.
    * @warning Does not clear the assigned namespaceId
    */
    void ClearPackages();

    /**
    * @brief Erase files associated with packages, file2PackageId, and zipFile2PackageId hashes.
    */
    void ErasePackage(const std::string& packageId);

    /**
    * @brief Erase files associated with packages, file2PackageId, and zipFile2PackageId hashes.
    * @warning Does not clear the assigned namespaceId
    */
    void ErasePackages();

    /**
    * @brief Returns the namespace for this package manager
    */
    const std::string GetNamespace();

    /**
    * @brief returns a string with the namespace suffix in the form `namespaceId/id`
    */
    const std::string WithNamespace(const std::string& id);

    private:
    std::string namespaceId;
    std::map<std::string, MetaClass*> packages;
    std::map<std::filesystem::path, std::string> filepathToPackageId;
    std::map<std::filesystem::path, std::string> zipFilepathToPackageId;
};

template<typename MetaClass>
void PackageManager<MetaClass>::LoadAllPackages(std::atomic<int>& progress)
{
  for (auto& [key, entry] : packages) {
    entry->OnMetaParsed();

    Logger::Logf(LogLevel::info, "Loaded package: %s", entry->packageId.c_str());

    progress++;
  }
}

template<typename MetaClass>
template<typename ScriptedDataType>
stx::result_t<std::string> PackageManager<MetaClass>::LoadPackageFromDisk(const std::filesystem::path& path)
{
#if defined(BN_MOD_SUPPORT)
  ResourceHandle handle;
  MetaClass* packageClass{ nullptr };

  auto modpath = std::filesystem::absolute(path).lexically_normal();

  std::string packageName = modpath.filename().generic_u8string();

  stx::result_t<sol::state*> res = handle.Scripts().LoadScript(namespaceId, modpath);

  if (!res.is_error()) {
    sol::state& state = *res.value();
    packageClass = this->CreatePackage<ScriptedDataType>(std::ref(state));

    //  Run all "includes" first
    if (state["package_requires_scripts"].valid()) {
      stx::result_t<sol::object> includesResult = CallLuaFunction(state, "package_requires_scripts");

      if (includesResult.is_error()) {
        delete packageClass;
        std::string msg = std::string("Failed to install package `") + packageName + "`. Reason: " + includesResult.error_cstr();
        return stx::error<std::string>(msg);
      }
    }

    // todo: use a ScopedWrapper
    stx::result_t<sol::object> initResult = CallLuaFunction(state, "package_init", packageClass);

    if (initResult.is_error()) {
      delete packageClass;
      std::string msg = std::string("Failed to install package `") + packageName + "`. Reason: " + initResult.error_cstr();
      return stx::error<std::string>(msg);
    }

    packageClass->OnMetaParsed();

    packageClass->SetFilePath(modpath);

    std::filesystem::path zip_path = modpath;
    zip_path.concat(".zip");

    stx::result_t<bool> zip_result = stx::zip(modpath, zip_path);
    if (zip_result.is_error()) {
      delete packageClass;
      std::string msg = std::string("Failed to install package `") + packageName + "`. Reason: " + zip_result.error_cstr();
      return stx::error<std::string>(msg);
    }

    stx::result_t<std::string> md5_result = stx::generate_md5_from_file(zip_path);
    if (md5_result.is_error()) {
      delete packageClass;
      std::string msg = std::string("Failed to install package `") + packageName + "`. Reason: " + md5_result.error_cstr();
      return stx::error<std::string>(msg);
    }
    else {
      packageClass->SetPackageFingerprint(md5_result.value());
    }

    if (stx::result_t<bool> commit_result = this->Commit(packageClass); commit_result.is_error()) {
      delete packageClass;
      std::string msg = std::string("Failed to install package `") + packageName + "`. Reason: " + commit_result.error_cstr();
      return stx::error<std::string>(msg);
    }
  }
  else {
    return stx::error<std::string>(res.error_cstr());
  }
#endif

  return stx::ok(packageClass->GetPackageID());
}

template<typename MetaClass>
template<typename ScriptedDataType>
stx::result_t<std::string> PackageManager<MetaClass>::LoadPackageFromZip(const std::filesystem::path& path)
{
#if defined(BN_MOD_SUPPORT)
  std::filesystem::path absolute = std::filesystem::absolute(path).make_preferred();
  std::filesystem::path extracted_path = absolute;
  extracted_path.remove_filename();
  extracted_path /= absolute.stem();

  std::filesystem::create_directory(extracted_path);  // Result is unused, do not fail if directory could not be crated (may already exist).
  auto result = stx::unzip(absolute, extracted_path);

  if (result.is_error()) {
    return stx::error<std::string>(result.error_cstr());
  }

  return this->LoadPackageFromDisk<ScriptedDataType>(extracted_path);
#elif
  return stx::error<std::string>("std::filesystem not supported");
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
inline std::string PackageManager<MetaClass>::FilepathToPackageID(const std::filesystem::path& file_path) {
  std::filesystem::path absolute = std::filesystem::absolute(file_path).lexically_normal();
  auto iter = filepathToPackageId.find(absolute);

  if (iter == filepathToPackageId.end()) {
    auto iter = zipFilepathToPackageId.find(absolute);
    if (iter == zipFilepathToPackageId.end()) {
      Logger::Log(LogLevel::debug, "Package manager could not find package with filepath " + file_path.u8string() + ". (also tested " + absolute.u8string() + ")");
      return "";
    }
    else {
      return iter->second;
    }
  }

  return iter->second;
}

template<typename MetaClass>
inline std::string PackageManager<MetaClass>::FilepathToPackageAddress(const std::filesystem::path& file_path) {
  auto packageId = FilepathToPackageID(file_path);

  if (packageId.empty()) {
    return packageId;
  }

  return WithNamespace(packageId);
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
inline const std::string PackageManager<MetaClass>::FilepathToPackageID(const std::filesystem::path& file_path) const {
  std::filesystem::path absolute = std::filesystem::absolute(file_path).lexically_normal();
  auto iter = filepathToPackageId.find(absolute);

  if (iter == filepathToPackageId.end()) {

    auto iter = zipFilepathToPackageId.find(absolute);
    if (iter == zipFilepathToPackageId.end()) {
      Logger::Log(LogLevel::debug, "Package manager could not find package with filepath " + file_path.u8string() + ". (also tested " + absolute.u8string() + ")");
      return "";
    }
    else {
      return iter->second;
    }
  }

  return iter->second;
}

template<typename MetaClass>
inline const std::string PackageManager<MetaClass>::FilepathToPackageAddress(const std::filesystem::path& file_path) const {
  auto packageId = FilepathToPackageID(file_path);

  if (packageId.empty()) {
    return packageId;
  }

  return WithNamespace(packageId);
}

template<typename MetaClass>
PackageManager<MetaClass>::~PackageManager<MetaClass>() {
  for (auto& [_, p] : packages) {
    delete p;
  }
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
  std::filesystem::path zipPath = package->GetFilePath();
  zipPath.concat(".zip");
  zipFilepathToPackageId.insert(std::make_pair(zipPath, packageId));
  return stx::ok();
}

template<typename MetaClass>
stx::result_t<std::filesystem::path> PackageManager<MetaClass>::RemovePackageByID(const std::string& id)
{
  if (auto iter = packages.find(id); iter != packages.end()) {
    std::filesystem::path path = iter->second->filepath;
    filepathToPackageId.erase(path);
    std::filesystem::path zipPath = path;
    zipPath.concat(".zip");
    zipFilepathToPackageId.erase(zipPath);

    delete iter->second;
    packages.erase(iter);

    return stx::ok<std::filesystem::path>(path);
  }

  return stx::error<std::filesystem::path>("No package with that ID");
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
stx::result_t<std::filesystem::path> PackageManager<MetaClass>::GetPackageFilePath(const std::string& id) const
{
  if (this->HasPackage(id)) {
    const MetaClass& meta = this->FindPackageByID(id);
    return stx::ok(meta.GetFilePath());
  }

  return stx::error<std::filesystem::path>("No package found");
}

template<typename MetaClass>
const unsigned PackageManager<MetaClass>::Size() const
{
  return (unsigned)packages.size();
}

template<typename MetaClass>
inline const std::string PackageManager<MetaClass>::GetNamespace()
{
  return namespaceId;
}

template<typename MetaClass>
inline const std::string PackageManager<MetaClass>::WithNamespace(const std::string& id)
{
  return PackageAddress{ namespaceId, id }; // implicit cast converts to formatted string
}

template<typename MetaClass>
inline void PackageManager<MetaClass>::ClearPackages()
{
  ResourceHandle handle;

  for (auto& [packageId, _] : this->packages) {
    PackageAddress addr = { GetNamespace(), packageId };
    handle.Scripts().DropPackageData(addr);
  }

  packages.clear();
  filepathToPackageId.clear();
  zipFilepathToPackageId.clear();
}

template<typename MetaClass>
inline void PackageManager<MetaClass>::ErasePackage(const std::string& packageId)
{
  auto packageIt = packages.find(packageId);

  if (packageIt == packages.end()) {
    Logger::Logf(LogLevel::debug, "Could not find package %s to erase", packageId.c_str());
    return;
  }

  MetaClass* package = packageIt->second;

  ResourceHandle handle;

  PackageAddress addr = { GetNamespace(), packageId };
  handle.Scripts().DropPackageData(addr);

  auto path = package->GetFilePath();

  std::filesystem::path absolute = std::filesystem::absolute(path);
  std::filesystem::path absoluteZip = absolute;
  absoluteZip.concat(".zip");

  std::filesystem::remove_all(absolute);
  std::filesystem::remove_all(absoluteZip);

  packages.erase(packageId);
  filepathToPackageId.erase(path);
  zipFilepathToPackageId.erase(path);
}

template<typename MetaClass>
inline void PackageManager<MetaClass>::ErasePackages()
{
  ResourceHandle handle;

  for (auto& [packageId, _] : this->packages) {
    PackageAddress addr = { GetNamespace(), packageId };
    handle.Scripts().DropPackageData(addr);
  }

  for (auto& [path, _] : filepathToPackageId) {
    std::filesystem::path absolute = std::filesystem::absolute(path);
    std::filesystem::remove_all(absolute);
  }

  for (auto& [path, _] : zipFilepathToPackageId) {
    std::filesystem::path absolute = std::filesystem::absolute(path);
    std::filesystem::remove_all(absolute);
  }

  packages.clear();
  filepathToPackageId.clear();
  zipFilepathToPackageId.clear();
}
