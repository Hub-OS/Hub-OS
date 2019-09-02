#include "bnMobRegistration.h"
#include "bnMobFactory.h"
#include "bnLogger.h"
#include <exception>
#include <atomic>
#include <thread>

MobRegistration::MobInfo::MobInfo() : placeholderTexture(nullptr)
{
  mobFactory = nullptr;
  name = "Unknown";
  description = "No description has been provided for this mob";
  atk = 0;
  speed = 0;
  hp = 0;
}

MobRegistration::MobInfo::~MobInfo()
{
  if (mobFactory) {
    delete mobFactory;
  }

  if (placeholderTexture) {
    delete placeholderTexture;
  }
}

MobRegistration::MobInfo& MobRegistration::MobInfo::SetPlaceholderTexturePath(std::string path)
{
  this->placeholderPath = path;
  return *this;
}

MobRegistration::MobInfo& MobRegistration::MobInfo::SetDescription(const std::string & message)
{
  this->description = message;
  return *this;
}

MobRegistration::MobInfo& MobRegistration::MobInfo::SetAttack(const int atk)
{
  this->atk = atk;
  return *this;
}

MobRegistration::MobInfo& MobRegistration::MobInfo::SetSpeed(const double speed)
{
  this->speed = speed;
  return *this;
}

MobRegistration::MobInfo& MobRegistration::MobInfo::SetHP(const int HP)
{
  hp = HP;
  return *this;
}

MobRegistration::MobInfo & MobRegistration::MobInfo::SetName(const std::string & name)
{
  this->name = name;
  return *this;
}

const sf::Texture* MobRegistration::MobInfo::GetPlaceholderTexture() const
{
  return this->placeholderTexture;
}

const std::string MobRegistration::MobInfo::GetPlaceholderTexturePath() const
{
  return this->placeholderPath;
}

const std::string MobRegistration::MobInfo::GetName() const
{
  return this->name;
}

const std::string MobRegistration::MobInfo::GetHPString() const
{
  return std::to_string(hp);
}

const std::string MobRegistration::MobInfo::GetDescriptionString() const
{
  return this->description;
}


const std::string MobRegistration::MobInfo::GetSpeedString() const
{
  std::string speedStr = std::to_string(speed);
  std::size_t afterDecimal = speedStr.find(".");
  speedStr = speedStr.substr(0, afterDecimal);
  return speedStr + "x";
}

const std::string MobRegistration::MobInfo::GetAttackString() const
{
  return std::to_string(atk);
}

Mob * MobRegistration::MobInfo::GetMob() const
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

void MobRegistration::Register(const MobInfo * info)
{
  roster.push_back(info);
}

const MobRegistration::MobInfo & MobRegistration::At(int index)
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

    Logger::GetMutex()->lock();
    Logger::Logf("Loaded mob: %s", roster[i]->GetName().c_str());
    Logger::GetMutex()->unlock();

    progress++;
  }
}
