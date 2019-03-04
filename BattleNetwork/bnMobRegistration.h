#pragma once

#include <map>
#include <vector>
#include <functional>
#include <atomic>
#include <SFML/Graphics.hpp>

#include "bnTextureResourceManager.h"
#include "bnField.h"

/*
  Use this singleton to register custom mobs  and have them automatically appear on the mob select screen and battle them!
*/

class MobFactory; // forward decl
class Mob;

typedef int SelectedMob;

class MobRegistration {
public:
  class MobInfo {
    friend class MobRegistration;

    MobFactory* mobFactory;
    sf::Sprite placeholder;
    std::string name;
    std::string description;
    std::string placeholderPath;
    sf::Texture* placeholderTexture;
    int atk;
    double speed;
    int hp;

    std::function<void()> loadMobClass;
  public:
    MobInfo();
    ~MobInfo();

    template<class T> MobInfo& SetMobClass();
    MobInfo& SetPlaceholderTexturePath(std::string path);
    MobInfo& SetDescription(const std::string& special);
    MobInfo& SetAttack(const int atk);
    MobInfo& SetSpeed(const double speed);
    MobInfo& SetHP(const int HP);
    MobInfo& SetName(const std::string& name);
    const sf::Texture* GetPlaceholderTexture() const;
    const std::string GetPlaceholderTexturePath() const;
    const std::string GetName() const;
    const std::string GetHPString() const;
    const std::string GetSpeedString() const;
    const std::string GetAttackString() const;
    const std::string GetDescriptionString() const;

    Mob* GetMob() const;
  };

private:
  std::vector<const MobInfo*> roster;

public:
  static MobRegistration &GetInstance();
  ~MobRegistration();

  template<class T>
  MobInfo* AddClass() {
    MobRegistration::MobInfo* info = new MobRegistration::MobInfo();
    info->SetMobClass<T>();
    this->Register(info);

    return info;
  }

  void Register(const MobInfo* info);
  const MobInfo& At(int index);
  const unsigned Size();
  void LoadAllMobs(std::atomic<int>& progress);

};

#define MOBS MobRegistration::GetInstance()

// Deffered loading design pattern
template<class T>
inline MobRegistration::MobInfo & MobRegistration::MobInfo::SetMobClass()
{
  loadMobClass = [this]() {
    if (mobFactory) {
      delete mobFactory;
      mobFactory = nullptr;
    }

    this->mobFactory = new T(new Field(6, 3));

    if (!this->placeholderTexture) {
      this->placeholderTexture = TEXTURES.LoadTextureFromFile(this->GetPlaceholderTexturePath());
    }
  };


  return *this;
}
