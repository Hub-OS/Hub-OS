#include "bnNaviRegistration.h"
#include "bnPlayer.h"
#include "bnLogger.h"
#include "stx/zip_utils.h"
#include <exception>
#include <atomic>
#include <thread>

#ifdef BN_MOD_SUPPORT
#include "bindings/bnScriptedPlayer.h"
#endif

NaviMeta::NaviMeta(): 
  iconTexture(), previewTexture(),
  PackageManager<NaviMeta>::Meta<Player>()
{
  special = "None";
  atk = 1;
  chargedAtk = 1;
  speed = 1;
  hp = 1;
  isSword = false;
}

NaviMeta::~NaviMeta()
{

}

void NaviMeta::OnPreGetData() {
  hp = data->GetHealth();
  data->SetAttackLevel(atk);
}

NaviMeta& NaviMeta::SetIconTexture(const std::shared_ptr<sf::Texture> icon)
{
  NaviMeta::iconTexture = icon;

  return *this;
}

NaviMeta& NaviMeta::SetSpecialDescription(const std::string && special)
{
  NaviMeta::special = special;
  return *this;
}

NaviMeta& NaviMeta::SetAttack(const unsigned atk)
{
  NaviMeta::atk = atk;
  return *this;
}

NaviMeta& NaviMeta::SetChargedAttack(const int atk)
{
  chargedAtk = atk;
  return *this;
}

NaviMeta& NaviMeta::SetSpeed(const double speed)
{
  NaviMeta::speed = speed;
  return *this;
}

NaviMeta& NaviMeta::SetHP(const int HP)
{
  hp = HP;
  return *this;
}

NaviMeta& NaviMeta::SetIsSword(const bool enabled)
{
  isSword = enabled;
  return *this;
}

NaviMeta& NaviMeta::SetMugshotAnimationPath(const std::string& path)
{
  mugshotAnimationPath = path;
  return *this;
}

NaviMeta& NaviMeta::SetMugshotTexturePath(const std::string& path)
{
  mugshotTexturePath = path;
  return *this;
}

NaviMeta &NaviMeta::SetEmotionsTexturePath(const std::string& texture)
{
  emotionsTexturePath = texture;
  return *this;
}

NaviMeta& NaviMeta::SetOverworldAnimationPath(const std::string& path)
{
  overworldAnimationPath = path;
  return *this;
}

NaviMeta& NaviMeta::SetOverworldTexturePath(const std::string& path)
{
  overworldTexturePath = path;
  return *this;
}

NaviMeta& NaviMeta::SetPreviewTexture(const std::shared_ptr<Texture> texture)
{
  previewTexture = texture;
  return *this;
}

const std::shared_ptr<Texture> NaviMeta::GetIconTexture() const
{
  return iconTexture;
}

const std::string& NaviMeta::GetOverworldTexturePath() const
{
  return overworldTexturePath;
}

const std::string& NaviMeta::GetOverworldAnimationPath() const 
{
  return overworldAnimationPath;
}

const std::string& NaviMeta::GetMugshotTexturePath() const
{
  return mugshotTexturePath;
}

const std::string& NaviMeta::GetMugshotAnimationPath() const
{
  return mugshotAnimationPath;
}

const std::string &NaviMeta::GetEmotionsTexturePath() const
{
  return emotionsTexturePath;
}

const std::shared_ptr<Texture> NaviMeta::GetPreviewTexture() const
{
  return previewTexture;
}

const std::string NaviMeta::GetName() const
{
  return data->GetName();
}

int NaviMeta::GetHP() const
{
  return hp;
}

const std::string NaviMeta::GetHPString() const
{
  return std::to_string(hp);
}

const std::string NaviMeta::GetSpeedString() const
{
  std::string speedStr = std::to_string(speed);
  std::size_t afterDecimal = speedStr.find(".");
  speedStr = speedStr.substr(0, afterDecimal);
  return speedStr + "x";
}

const Element NaviMeta::GetElement() const
{
  return data->GetElement();
}

const std::string NaviMeta::GetAttackString() const
{
  return std::to_string(atk) + "-" + std::to_string(chargedAtk) + " charged";
}

const std::string& NaviMeta::GetSpecialDescriptionString() const
{
  return special;
}