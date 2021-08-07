#include "bnNaviRegistration.h"
#include "bnPlayer.h"
#include "bnLogger.h"
#include <exception>
#include <atomic>
#include <thread>

NaviRegistration::NaviMeta::NaviMeta() : iconTexture(), previewTexture()
{
  navi = nullptr;
  special = "None";
  atk = 1;
  chargedAtk = 1;
  speed = 1;
  hp = 1;
  isSword = false;
}

NaviRegistration::NaviMeta::~NaviMeta()
{
  if (navi != nullptr) {
    delete navi;
  }
}

NaviRegistration::NaviMeta& NaviRegistration::NaviMeta::SetIconTexture(const std::shared_ptr<sf::Texture> icon)
{
  NaviMeta::iconTexture = icon;

  return *this;
}

const std::string& NaviRegistration::NaviMeta::GetPackageID() const
{
  return packageId;
}

NaviRegistration::NaviMeta& NaviRegistration::NaviMeta::SetPackageID(const std::string& id)
{
  NaviMeta::packageId = id;

  return *this;
}

NaviRegistration::NaviMeta& NaviRegistration::NaviMeta::SetSpecialDescription(const std::string && special)
{
  NaviMeta::special = special;
  return *this;
}

NaviRegistration::NaviMeta& NaviRegistration::NaviMeta::SetAttack(const unsigned atk)
{
  NaviMeta::atk = atk;
  return *this;
}

NaviRegistration::NaviMeta& NaviRegistration::NaviMeta::SetChargedAttack(const int atk)
{
  chargedAtk = atk;
  return *this;
}

NaviRegistration::NaviMeta& NaviRegistration::NaviMeta::SetSpeed(const double speed)
{
  NaviMeta::speed = speed;
  return *this;
}

NaviRegistration::NaviMeta& NaviRegistration::NaviMeta::SetHP(const int HP)
{
  hp = HP;
  return *this;
}

NaviRegistration::NaviMeta& NaviRegistration::NaviMeta::SetIsSword(const bool enabled)
{
  isSword = enabled;
  return *this;
}

NaviRegistration::NaviMeta& NaviRegistration::NaviMeta::SetMugshotAnimationPath(const std::string& path)
{
  mugshotAnimationPath = path;
  return *this;
}

NaviRegistration::NaviMeta& NaviRegistration::NaviMeta::SetMugshotTexturePath(const std::string& path)
{
  mugshotTexturePath = path;
  return *this;
}

NaviRegistration::NaviMeta &NaviRegistration::NaviMeta::SetEmotionsTexturePath(const std::string& texture)
{
  emotionsTexturePath = texture;
  return *this;
}

NaviRegistration::NaviMeta& NaviRegistration::NaviMeta::SetOverworldAnimationPath(const std::string& path)
{
  overworldAnimationPath = path;
  return *this;
}

NaviRegistration::NaviMeta& NaviRegistration::NaviMeta::SetOverworldTexturePath(const std::string& path)
{
  overworldTexturePath = path;
  return *this;
}

NaviRegistration::NaviMeta& NaviRegistration::NaviMeta::SetPreviewTexture(const std::shared_ptr<Texture> texture)
{
  previewTexture = texture;
  return *this;
}

const std::shared_ptr<Texture> NaviRegistration::NaviMeta::GetIconTexture() const
{
  return iconTexture;
}

const std::string& NaviRegistration::NaviMeta::GetOverworldTexturePath() const
{
  return overworldTexturePath;
}

const std::string& NaviRegistration::NaviMeta::GetOverworldAnimationPath() const 
{
  return overworldAnimationPath;
}

const std::string& NaviRegistration::NaviMeta::GetMugshotTexturePath() const
{
  return mugshotTexturePath;
}

const std::string& NaviRegistration::NaviMeta::GetMugshotAnimationPath() const
{
  return mugshotAnimationPath;
}

const std::string &NaviRegistration::NaviMeta::GetEmotionsTexturePath() const
{
  return emotionsTexturePath;
}

const std::shared_ptr<Texture> NaviRegistration::NaviMeta::GetPreviewTexture() const
{
  return previewTexture;
}

const std::string NaviRegistration::NaviMeta::GetName() const
{
  return navi->GetName();
}

int NaviRegistration::NaviMeta::GetHP() const
{
  return hp;
}

const std::string NaviRegistration::NaviMeta::GetHPString() const
{
  return std::to_string(hp);
}

const std::string NaviRegistration::NaviMeta::GetSpeedString() const
{
  std::string speedStr = std::to_string(speed);
  std::size_t afterDecimal = speedStr.find(".");
  speedStr = speedStr.substr(0, afterDecimal);
  return speedStr + "x";
}

const Element NaviRegistration::NaviMeta::GetElement() const
{
  return navi->GetElement();
}

const std::string NaviRegistration::NaviMeta::GetAttackString() const
{
  return std::to_string(atk) + "-" + std::to_string(chargedAtk) + " charged";
}

const std::string& NaviRegistration::NaviMeta::GetSpecialDescriptionString() const
{
  return special;
}

Player * NaviRegistration::NaviMeta::GetNavi()
{
  Player* out = navi;
  navi = nullptr;

  loadNaviClass(); // Reload navi (which restores HP)

  out->SetAttackLevel(this->atk);

  return out;
}

NaviRegistration & NaviRegistration::GetInstance()
{
 static NaviRegistration singleton; return singleton; 
}

NaviRegistration::~NaviRegistration()
{
  for (auto& [_, entry] : roster) {
    delete entry;
  }

  roster.clear();
}

stx::result_t<bool> NaviRegistration::Commit(NaviMeta * info)
{
  std::string packageId = info->GetPackageID();
  if (info && !packageId.empty()) {
    if (roster.find(packageId) == roster.end()) {
      roster.insert(std::make_pair(packageId, info));
    }
    else {
      return stx::error<bool>(std::string("There is already a package in the player roster with id of ") + packageId);
    }
  }
  else {
    return stx::error<bool>(std::string("info object was nullptr or package ID was not set"));
  }

  return stx::ok(true);
}

NaviRegistration::NaviMeta & NaviRegistration::FindByPackageID(const std::string& id)
{
  auto iter = roster.find(id);

  if(iter == roster.end()) {
    std::string error = "Roster could not find package ID " + id;
    throw std::runtime_error(error);
  }

  return *(iter->second);
}

bool NaviRegistration::HasPackage(const std::string& id)
{
  return roster.find(id) != roster.end();
}

const std::string NaviRegistration::FirstValidPackage()
{
  if (roster.empty()) {
    throw std::runtime_error("Player package list is empty!");
  }

  return roster.begin()->first;
}

const std::string NaviRegistration::GetPackageBefore(const std::string& id)
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

const std::string NaviRegistration::GetPackageAfter(const std::string& id)
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

const unsigned NaviRegistration::Size()
{
  return (unsigned)roster.size();
}

void NaviRegistration::LoadAllNavis(std::atomic<int>& progress)
{
  for (auto& [key, entry]: roster) {
    entry->loadNaviClass();

    Logger::Logf("Loaded player package: %s", entry->navi->GetName().c_str());

    progress++;
  }
}
