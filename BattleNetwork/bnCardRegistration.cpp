#include "bnCardRegistration.h"
#include "bnPlayer.h"
#include "bnLogger.h"
#include "stx/zip_utils.h"
#include <exception>
#include <atomic>
#include <thread>

#ifdef BN_MOD_SUPPORT
#include "bindings/bnScriptedCard.h"
#endif

CardRegistration::CardMeta::CardMeta() : iconTexture(), previewTexture()
{ }

CardRegistration::CardMeta::~CardMeta()
{
  if (card != nullptr) {
    delete card;
  }
}

CardRegistration::CardMeta& CardRegistration::CardMeta::SetIconTexture(const std::shared_ptr<sf::Texture> icon)
{
  CardMeta::iconTexture = icon;

  return *this;
}

const std::string& CardRegistration::CardMeta::GetPackageID() const
{
  return packageId;
}

const std::string& CardRegistration::CardMeta::GetFilePath() const
{
  return filepath;
}

CardRegistration::CardMeta& CardRegistration::CardMeta::SetPackageID(const std::string& id)
{
  CardMeta::packageId = id;

  return *this;
}

CardRegistration::CardMeta& CardRegistration::CardMeta::SetFilePath(const std::string& filepath)
{
  CardMeta::filepath = filepath;

  return *this;
}

CardRegistration::CardMeta& CardRegistration::CardMeta::SetPreviewTexture(const std::shared_ptr<Texture> texture)
{
  previewTexture = texture;
  return *this;
}

const std::shared_ptr<Texture> CardRegistration::CardMeta::GetIconTexture() const
{
  return iconTexture;
}

const std::shared_ptr<Texture> CardRegistration::CardMeta::GetPreviewTexture() const
{
  return previewTexture;
}

Battle::Card::Properties& CardRegistration::CardMeta::GetCardProperties()
{
  return properties;
}

CardImpl *CardRegistration::CardMeta::GetCard()
{
  CardImpl* out = card;;
  card = nullptr;

  loadClass(); // Reload class

  return out;
}


CardRegistration::~CardRegistration()
{
  for (auto& [_, entry] : roster) {
    delete entry;
  }

  roster.clear();
}

stx::result_t<bool> CardRegistration::Commit(CardMeta* info)
{
  std::string packageId = info->GetPackageID();
  if (info && !packageId.empty()) {
    if (roster.find(packageId) == roster.end()) {
      roster.insert(std::make_pair(packageId, info));
    }
    else {
      return stx::error<bool>(std::string("There is already a package in the card roster with id of ") + packageId);
    }
  }
  else {
    return stx::error<bool>(std::string("info object was nullptr or package ID was not set"));
  }

  info->loadClass();

  return stx::ok();
}

CardRegistration::CardMeta& CardRegistration::FindByPackageID(const std::string& id)
{
  auto iter = roster.find(id);

  if (iter == roster.end()) {
    std::string error = "Roster could not find package ID " + id;
    throw std::runtime_error(error);
  }

  return *(iter->second);
}

stx::result_t<std::string> CardRegistration::RemovePackageByID(const std::string& id)
{
  if (auto iter = roster.find(id); iter != roster.end()) {
    std::string path = iter->second->filepath;
    roster.erase(iter);
    return stx::ok(path);
  }

  return stx::error<std::string>("No package with that ID");
}

bool CardRegistration::HasPackage(const std::string& id)
{
  return roster.find(id) != roster.end();
}

const std::string CardRegistration::FirstValidPackage()
{
  if (roster.empty()) {
    throw std::runtime_error("Card package list is empty!");
  }

  return roster.begin()->first;
}

const std::string CardRegistration::GetPackageBefore(const std::string& id)
{
  std::string previous_key;

  for (auto iter = roster.begin(); iter != roster.end(); iter = std::next(iter)) {
    std::string key = iter->first;

    if (key == id) {
      if (previous_key.empty()) {
        previous_key = roster.rbegin()->first;
      }

      break;
    }

    previous_key = key;
  }

  return previous_key;
}

const std::string CardRegistration::GetPackageAfter(const std::string& id)
{
  std::string previous_key;

  for (auto iter = roster.rbegin(); iter != roster.rend(); iter = std::next(iter)) {
    std::string key = iter->first;

    if (key == id) {
      if (previous_key.empty()) {
        previous_key = roster.begin()->first;
      }

      break;
    }

    previous_key = key;
  }

  return previous_key;
}

stx::result_t<std::string> CardRegistration::GetPackageFilePath(const std::string& id)
{
  if (this->HasPackage(id)) {
    auto& meta = this->FindByPackageID(id);
    return stx::ok(meta.GetFilePath());
  }

  return stx::error<std::string>("No package found");
}

const unsigned CardRegistration::Size()
{
  return (unsigned)roster.size();
}

void CardRegistration::LoadAllCards(std::atomic<int>& progress)
{
  for (auto& [key, entry] : roster) {
    entry->loadClass();
    auto& props = entry->GetCardProperties();

    Logger::Logf("Loaded card package: %s", props.shortname.c_str());

    progress++;
  }
}

stx::result_t<bool> CardRegistration::LoadCardFromPackage(const std::string& path)
{
#if defined(BN_MOD_SUPPORT) && !defined(__APPLE__)
  ResourceHandle handle;

  const auto& modpath = std::filesystem::absolute(path);
  auto entrypath = modpath / "entry.lua";
  std::string packageName = modpath.filename().string();
  auto& res = handle.Scripts().LoadScript(entrypath.string());

  if (res.result.valid()) {
    sol::state& state = *res.state;
    auto customInfo = this->AddClass<ScriptedCard>(std::ref(state));

    // Sets the predefined _modpath variable
    state["_modpath"] = modpath.string() + "/";

    // run script on meta info object
    state["roster_init"](customInfo);

    customInfo->GetCardProperties().uuid = customInfo->GetPackageID();
    customInfo->SetFilePath(modpath.string());
    return this->Commit(customInfo);

  }
  else {
    sol::error sol_error = res.result;
    std::string msg = std::string("Failed to load card mod ") + packageName + ". Reason: " + sol_error.what();
    return stx::error<bool>(msg);
  }
#endif

  return stx::ok();
}

stx::result_t<bool> CardRegistration::LoadCardFromZip(const std::string& path)
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
    return LoadCardFromPackage(extracted_path);
  }

  return result;
#elif
  return stx::error<bool>("std::filesystem not supported");
#endif
}
