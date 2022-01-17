#include "bnMobPackageManager.h"
#include "bnMobFactory.h"
#include "bnLogger.h"
#include <exception>
#include <atomic>
#include <thread>

MobMeta::MobMeta():
  placeholderTexture(nullptr),
  PackageManager<MobMeta>::Meta<MobFactory>()
{
  name = "Unknown";
  description = "No description has been provided for this mob";
  atk = 0;
  speed = 0;
  hp = 0;
}

MobMeta::~MobMeta()
{
}

void MobMeta::OnMetaParsed()
{
  static ResourceHandle handle;
  placeholderTexture = handle.Textures().LoadFromFile(placeholderPath);
}

MobMeta& MobMeta::SetPlaceholderTexturePath(const std::filesystem::path& path)
{
  placeholderPath = path;
  return *this;
}

MobMeta& MobMeta::SetDescription(const std::string & message)
{
  description = message;
  return *this;
}

MobMeta& MobMeta::SetAttack(const int atk)
{
  MobMeta::atk = atk;
  return *this;
}

MobMeta& MobMeta::SetSpeed(const double speed)
{
  MobMeta::speed = speed;
  return *this;
}

MobMeta& MobMeta::SetHP(const int HP)
{
  hp = HP;
  return *this;
}

MobMeta & MobMeta::SetName(const std::string & name)
{
  MobMeta::name = name;
  return *this;
}

const std::shared_ptr<sf::Texture> MobMeta::GetPlaceholderTexture() const
{
  return placeholderTexture;
}

const std::filesystem::path MobMeta::GetPlaceholderTexturePath() const
{
  return placeholderPath;
}

const std::string MobMeta::GetName() const
{
  return name;
}

const std::string MobMeta::GetHPString() const
{
  return std::to_string(hp);
}

const std::string MobMeta::GetDescriptionString() const
{
  return description;
}

const std::string MobMeta::GetSpeedString() const
{
  std::string speedStr = std::to_string(speed);
  std::size_t afterDecimal = speedStr.find(".");
  speedStr = speedStr.substr(0, afterDecimal);
  return speedStr + "x";
}

const std::string MobMeta::GetAttackString() const
{
  return std::to_string(atk);
}
