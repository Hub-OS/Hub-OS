#include "bnMobRegistration.h"
#include "bnMobFactory.h"
#include "bnLogger.h"
#include <exception>
#include <atomic>
#include <thread>

MobRegistration::MobMeta::MobMeta() : placeholderTexture(nullptr)
{
  mobFactory = nullptr;
  name = "Unknown";
  description = "No description has been provided for this mob";
  atk = 0;
  speed = 0;
  hp = 0;
}

MobRegistration::MobMeta::~MobMeta()
{
  if (mobFactory) {
    delete mobFactory;
  }

  if (placeholderTexture) {
    delete placeholderTexture;
  }
}

MobRegistration::MobMeta& MobRegistration::MobMeta::SetPlaceholderTexturePath(std::string path)
{
  this->placeholderPath = path;
  return *this;
}

MobRegistration::MobMeta& MobRegistration::MobMeta::SetDescription(const std::string & message)
{
  this->description = message;
  return *this;
}

MobRegistration::MobMeta& MobRegistration::MobMeta::SetAttack(const int atk)
{
  this->atk = atk;
  return *this;
}

MobRegistration::MobMeta& MobRegistration::MobMeta::SetSpeed(const double speed)
{
  this->speed = speed;
  return *this;
}

MobRegistration::MobMeta& MobRegistration::MobMeta::SetHP(const int HP)
{
  hp = HP;
  return *this;
}

MobRegistration::MobMeta & MobRegistration::MobMeta::SetName(const std::string & name)
{
  this->name = name;
  return *this;
}

const sf::Texture* MobRegistration::MobMeta::GetPlaceholderTexture() const
{
  return this->placeholderTexture;
}

const std::string MobRegistration::MobMeta::GetPlaceholderTexturePath() const
{
  return this->placeholderPath;
}

const std::string MobRegistration::MobMeta::GetName() const
{
  return this->name;
}

const std::string MobRegistration::MobMeta::GetHPString() const
{
  return std::to_string(hp);
}

const std::string MobRegistration::MobMeta::GetDescriptionString() const
{
  return this->description;
}


const std::string MobRegistration::MobMeta::GetSpeedString() const
{
  std::string speedStr = std::to_string(speed);
  std::size_t afterDecimal = speedStr.find(".");
  speedStr = speedStr.substr(0, afterDecimal);
  return speedStr + "x";
}

const std::string MobRegistration::MobMeta::GetAttackString() const
{
  return std::to_string(atk);
}

Mob * MobRegistration::MobMeta::GetMob() const
{
  loadMobClass(); // Reload mob
  return mobFactory->Build();
}

MobRegistration & MobRegistration::GetInstance()
{
  static MobRegistration singleton; return singleton;
}

MobRegistration::~MobRegistration()
{
  for (int i = 0; i < (int)Size(); i++) {
    delete roster[i];
  }

  roster.clear();
}

void MobRegistration::Register(const MobMeta * info)
{
  roster.push_back(info);
}

const MobRegistration::MobMeta & MobRegistration::At(int index)
{
  if (index < 0 || index >= (int)Size())
    throw std::runtime_error("Roster index out of bounds");

  return *(roster.at(index));
}

const unsigned MobRegistration::Size()
{
  return (unsigned)roster.size();
}

void MobRegistration::LoadAllMobs(std::atomic<int>& progress)
{
  for (int i = 0; i < (int)Size(); i++) {
    roster[i]->loadMobClass();

    Logger::Logf("Loaded mob: %s", roster[i]->GetName().c_str());

    progress++;
  }
}
