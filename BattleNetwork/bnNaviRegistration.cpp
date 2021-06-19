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

const std::string NaviRegistration::NaviMeta::GetOverworldTexturePath() const
{
  return overworldTexturePath;
}

const std::string & NaviRegistration::NaviMeta::GetOverworldAnimationPath() const 
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

const std::string NaviRegistration::NaviMeta::GetSpecialDescriptionString() const
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
  for (int i = 0; i < (int)Size(); i++) {
    delete roster[i];
  }

  roster.clear();
}

void NaviRegistration::Register(NaviMeta * info)
{
  roster.push_back(info);
}

NaviRegistration::NaviMeta & NaviRegistration::At(int index)
{
  if (index < 0 || index >= (int)Size()) {
    Logger::Logf("Roster index out of bounds. Reverting to index #0");
    index = 0;
  }

  return *(roster.at(index));
}

const unsigned NaviRegistration::Size()
{
  return (unsigned)roster.size();
}

void NaviRegistration::LoadAllNavis(std::atomic<int>& progress)
{
  for (int i = 0; i < (int)Size(); i++) {
    roster[i]->loadNaviClass();

    Logger::Logf("Loaded navi: %s", roster[i]->navi->GetName().c_str());

    progress++;
  }
}
