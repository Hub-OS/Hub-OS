#include "bnPlayerPackageManager.h"
#include "bnPlayer.h"
#include "bnLogger.h"
#include "stx/zip_utils.h"
#include <exception>
#include <atomic>
#include <thread>

#ifdef BN_MOD_SUPPORT
#include "bindings/bnScriptedPlayer.h"
#endif

PlayerMeta::PlayerMeta():
  iconTexture(), previewTexture(),
  PackageManager<PlayerMeta>::Meta<Player>()
{
  special = "None";
  atk = 1;
  chargedAtk = 1;
  speed = 1;
  isSword = false;
}

PlayerMeta::~PlayerMeta()
{
}

void PlayerMeta::OnMetaParsed() {
  if (data == nullptr) {
    return;
  }

  auto player = std::shared_ptr<Player>(data);
  player->Init();
  element = player->GetElement();
  name = player->GetName();
  hp = player->GetHealth();
  loadClass();
}

void PlayerMeta::PreGetData() {
  data->SetAttackLevel(atk);
}

PlayerMeta& PlayerMeta::SetIconTexture(const std::shared_ptr<sf::Texture> icon)
{
  PlayerMeta::iconTexture = icon;

  return *this;
}

PlayerMeta& PlayerMeta::SetSpecialDescription(const std::string && special)
{
  PlayerMeta::special = special;
  return *this;
}

PlayerMeta& PlayerMeta::SetAttack(const unsigned atk)
{
  PlayerMeta::atk = atk;
  return *this;
}

PlayerMeta& PlayerMeta::SetChargedAttack(const int atk)
{
  chargedAtk = atk;
  return *this;
}

PlayerMeta& PlayerMeta::SetSpeed(const double speed)
{
  PlayerMeta::speed = speed;
  return *this;
}

PlayerMeta& PlayerMeta::SetHP(const int HP)
{
  hp = HP;
  return *this;
}

PlayerMeta& PlayerMeta::SetIsSword(const bool enabled)
{
  isSword = enabled;
  return *this;
}

PlayerMeta& PlayerMeta::SetMugshotAnimationPath(const std::filesystem::path& path)
{
  mugshotAnimationPath = path;
  return *this;
}

PlayerMeta& PlayerMeta::SetMugshotTexturePath(const std::filesystem::path& path)
{
  mugshotTexturePath = path;
  return *this;
}

PlayerMeta& PlayerMeta::SetEmotionsTexturePath(const std::filesystem::path& path)
{
  emotionsTexturePath = path;
  return *this;
}

PlayerMeta& PlayerMeta::SetOverworldAnimationPath(const std::filesystem::path& path)
{
  overworldAnimationPath = path;
  return *this;
}

PlayerMeta& PlayerMeta::SetOverworldTexturePath(const std::filesystem::path& path)
{
  overworldTexturePath = path;
  return *this;
}

PlayerMeta& PlayerMeta::SetPreviewTexture(const std::shared_ptr<Texture> texture)
{
  previewTexture = texture;
  return *this;
}

const std::shared_ptr<Texture> PlayerMeta::GetIconTexture() const
{
  return iconTexture;
}

const std::filesystem::path& PlayerMeta::GetOverworldTexturePath() const
{
  return overworldTexturePath;
}

const std::filesystem::path& PlayerMeta::GetOverworldAnimationPath() const
{
  return overworldAnimationPath;
}

const std::filesystem::path& PlayerMeta::GetMugshotTexturePath() const
{
  return mugshotTexturePath;
}

const std::filesystem::path& PlayerMeta::GetMugshotAnimationPath() const
{
  return mugshotAnimationPath;
}

const std::filesystem::path & PlayerMeta::GetEmotionsTexturePath() const
{
  return emotionsTexturePath;
}

const std::shared_ptr<Texture> PlayerMeta::GetPreviewTexture() const
{
  return previewTexture;
}

const std::string PlayerMeta::GetName() const
{
  return name;
}

int PlayerMeta::GetHP() const
{
  return hp;
}

const std::string PlayerMeta::GetHPString() const
{
  return std::to_string(hp);
}

const std::string PlayerMeta::GetSpeedString() const
{
  std::string speedStr = std::to_string(speed);
  std::size_t afterDecimal = speedStr.find(".");
  speedStr = speedStr.substr(0, afterDecimal);
  return speedStr + "x";
}

const Element PlayerMeta::GetElement() const
{
  return element;
}

const std::string PlayerMeta::GetAttackString() const
{
  return std::to_string(atk) + "-" + std::to_string(chargedAtk) + " charged";
}

const std::string& PlayerMeta::GetSpecialDescriptionString() const
{
  return special;
}
