#include "bnNaviRegistration.h"
#include "bnPlayer.h"
#include "bnLogger.h"
#include <exception>
#include <atomic>
#include <thread>

NaviRegistration::NaviMeta::NaviMeta() : symbol(), overworldTexture(), battleTexture()
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

NaviRegistration::NaviMeta& NaviRegistration::NaviMeta::SetSymbolTexture(sf::Texture& symbol)
{
  this->symbol = sf::Sprite(symbol);

  // Symbols are 15x15
  this->symbol.setTextureRect(sf::IntRect(0, 0, 15, 15));

  return *this;
}

NaviRegistration::NaviMeta& NaviRegistration::NaviMeta::SetSpecialDescription(const std::string && special)
{
  this->special = special;
  return *this;
}

NaviRegistration::NaviMeta& NaviRegistration::NaviMeta::SetAttack(const int atk)
{
  this->atk = atk;
  return *this;
}

NaviRegistration::NaviMeta& NaviRegistration::NaviMeta::SetChargedAttack(const int atk)
{
  this->chargedAtk = atk;
  return *this;
}

NaviRegistration::NaviMeta& NaviRegistration::NaviMeta::SetSpeed(const double speed)
{
  this->speed = speed;
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

NaviRegistration::NaviMeta& NaviRegistration::NaviMeta::SetOverworldAnimationPath(const std::string && path)
{
  overworldAnimationPath = path;
  return *this;
}

NaviRegistration::NaviMeta& NaviRegistration::NaviMeta::SetOverworldTexture(const sf::Texture * texture)
{
  overworldTexture = const_cast<sf::Texture*>(texture);
  return *this;
}

NaviRegistration::NaviMeta& NaviRegistration::NaviMeta::SetBattleAnimationPath(const std::string && path)
{
  battleAnimationPath = path;
  return *this;
}

NaviRegistration::NaviMeta& NaviRegistration::NaviMeta::SetBattleTexture(const sf::Texture * texture)
{
  battleTexture = const_cast<sf::Texture*>(texture);
  return *this;
}

const sf::Texture & NaviRegistration::NaviMeta::GetOverworldTexture() const
{
  return *overworldTexture;
}

const std::string & NaviRegistration::NaviMeta::GetOverworldAnimationPath() const 
{
  return overworldAnimationPath;
}

const sf::Texture & NaviRegistration::NaviMeta::GetBattleTexture() const
{
  return *battleTexture;
}

const std::string& NaviRegistration::NaviMeta::GetBattleAnimationPath() const
{
  return battleAnimationPath;
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
  return std::to_string(atk) + " - " + std::to_string(chargedAtk) + " charged";
}

const std::string NaviRegistration::NaviMeta::GetSpecialDescriptionString() const
{
  return special;
}

Player * NaviRegistration::NaviMeta::GetNavi()
{
  Player* out = navi;
  navi = nullptr;

  loadNaviClass(); // Reload navi and restore HP 

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
  if (index < 0 || index >= (int)Size())
    throw std::runtime_error("Roster index out of bounds");

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

    Logger::GetMutex()->lock();
    Logger::Logf("Loaded navi: %s", roster[i]->navi->GetName().c_str());
    Logger::GetMutex()->unlock();

    progress++;
  }
}
