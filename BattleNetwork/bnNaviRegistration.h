#pragma once

#include <map>
#include <vector>
#include <functional>
#include <atomic>
#include <SFML/Graphics.hpp>
#include "bnElements.h"
#include "bnPlayer.h"

/*
  Use this singleton to register custom navis and have them automatically appear on the select, overworld, and battle scenes
*/

class Player; // forward decl

typedef int SelectedNavi;

class NaviRegistration {
public:
  class NaviInfo {
    friend class NaviRegistration;

    Player* navi;
    sf::Sprite symbol;
    std::string special;
    std::string overworldAnimationPath;
    std::string battleAnimationPath;
    std::string name;
    sf::Texture* overworldTexture;
    sf::Texture* battleTexture;
    int atk;
    int chargedAtk;
    double speed;
    int hp;
    bool isSword;

    std::function<void()> loadNaviClass;
  public:
    NaviInfo();
    ~NaviInfo();

    template<class T> NaviInfo& SetNaviClass();
    NaviInfo& SetSymbolTexture(sf::Texture& symbol);
    NaviInfo& SetSpecialDescription(const std::string&& special);
    NaviInfo& SetAttack(const int atk);
    NaviInfo& SetChargedAttack(const int atk);
    NaviInfo& SetSpeed(const double speed);
    NaviInfo& SetHP(const int HP);
    NaviInfo& SetIsSword(const bool enabled);
    NaviInfo& SetOverworldAnimationPath(const std::string&& path);
    NaviInfo& SetOverworldTexture(const sf::Texture* texture);
    NaviInfo& SetBattleAnimationPath(const std::string&& path);
    NaviInfo& SetBattleTexture(const sf::Texture* texture);
    const sf::Texture& GetOverworldTexture() const;
    const std::string& GetOverworldAnimationPath() const;
    const sf::Texture& GetBattleTexture() const;
    const std::string& GetBattleAnimationPath() const;
    const std::string GetName() const;
    const std::string GetHPString() const;
    const std::string GetSpeedString() const;
    const std::string GetAttackString() const;
    const std::string GetSpecialDescriptionString() const;
    const Element GetElement() const;

    Player* GetNavi();
  };

private:
  std::vector<NaviInfo*> roster;

public:
  static NaviRegistration &GetInstance();
  ~NaviRegistration();

  template<class T>
  NaviInfo* AddClass() {
    NaviRegistration::NaviInfo* info = new NaviRegistration::NaviInfo();
    info->SetNaviClass<T>();
    this->Register(info);

    return info;
  }

  void Register(NaviInfo* info);
  NaviInfo& At(int index);
  const unsigned Size();
  void LoadAllNavis(std::atomic<int>& progress);
  
};

#define NAVIS NaviRegistration::GetInstance()

// Deffered loading design pattern
template<class T>
inline NaviRegistration::NaviInfo & NaviRegistration::NaviInfo::SetNaviClass()
{
  loadNaviClass = [this]() { 
    this->navi = new T(); 
    this->battleTexture = const_cast<sf::Texture*>(this->navi->getTexture());
    this->overworldTexture = const_cast<sf::Texture*>(this->navi->getTexture());
    this->hp = this->navi->GetHealth();
  };


  return *this;
}
