
/*! \file bnMobRegistration.h */

#pragma once

#include <map>
#include <vector>
#include <functional>
#include <atomic>
#include <SFML/Graphics.hpp>

#include "bnTextureResourceManager.h"
#include "bnField.h"
#include "bnMobFactory.h"

class Mob;

typedef int SelectedMob;

/*! \brief Use this singleton to register custom mobs and have them automatically appear on the mob select screen and battle them!
*/
class MobRegistration {
public:
  /*! \brief Mob info roster */
  class MobMeta {
    friend class MobRegistration;

    MobFactory* mobFactory; /*!< The mob factory to build the mob */
    sf::Sprite placeholder; /*!< Preview image for the mob in select screen */
    std::string name;       /*!< Name of the mob */
    std::string description;/*!< Description of mob that shows up in the text box */
    std::string placeholderPath; /*!< Path to the preview image */
    std::shared_ptr<sf::Texture> placeholderTexture; /*!< Texture of the preview image */
    int atk; /*!< Strength of mob to display */
    double speed; /*!< Speed of mob to display */
    int hp; /*!< Total health of mob to display */

    std::function<void()> loadMobClass; /*!< Deferred mob loader function */
    public:
    /**
     * @brief Sets mob to temp data
     */ 
    MobMeta();
    
    /**
     * @brief deconstructor
     */
    ~MobMeta();

    /**
     * @brief Initializes the deffered loader
     * @return MobMeta& to chain
     */
    template<class T> MobMeta& SetMobClass();
    
    /**
     * @brief Sets the preview path to load
     * @param path path to preview texture
     * @return MobMeta& to chain
     */
    MobMeta& SetPlaceholderTexturePath(std::string path);
    
    /**
     * @brief Sets the description that shows up in text box
     * @param message 
     * @return MobMeta& to chain
     */
    MobMeta& SetDescription(const std::string& message);
    
    /**
     * @brief Sets the attack to display on the select screen
     * @param atk strength of the mob
     * @return MobMeta& to chain
     */
    MobMeta& SetAttack(const int atk);
    
    /**
     * @brief Sets the speed to display on the select screen
     * @param speed of the mob
     * @return MobMeta& to chain
     */
    MobMeta& SetSpeed(const double speed);
    
    /**
     * @brief Sets the total HP of the mob
     * @param HP health of the mob to display
     * @return MobMeta& to chain
     */
    MobMeta& SetHP(const int HP);
    
    /**
     * @brief Clever title of the mob
     * @param name name of the mob to display
     * @return MobMeta& to chain
     */
    MobMeta& SetName(const std::string& name);
    
    /**
     * @brief Gets the preview texture
     * @return const std::shared_ptr<sf::Texture>
     */
    const std::shared_ptr<sf::Texture> GetPlaceholderTexture() const;
    
    /**
     * @brief Gets the preview texture path 
     * @return const std::string
     */
    const std::string GetPlaceholderTexturePath() const;
    
    /**
     * @brief Gets the name of the mob 
     * @return const std::string
     */
    const std::string GetName() const;
    
    /**
     * @brief Gets the health of the mob as a string
     * @return const std::string
     */
    const std::string GetHPString() const;
    
    /**
     * @brief Gets the speed of the mob as a string
     * @return const std::string
     */
    const std::string GetSpeedString() const;
    
    /**
     * @brief Gets the attack of the mob as a string
     * @return const std::string
     */
    const std::string GetAttackString() const;
    
    /**
     * @brief Gets the mob description to show up in message box
     * @return const std::string
     */
    const std::string GetDescriptionString() const;

    /**
     * @brief Uses deferred loader to load MobFactory and build the mob
     * @return Mob* to send to BattleScene
     */
    Mob* GetMob() const;
  };

private:
  std::vector<const MobMeta*> roster; /*!< List of all mobs registered */

  void Register(const MobMeta* info);
public:
  /**
   * @brief Gets singleton of the resource
   * @return MobRegistration& singleton 
   */
  static MobRegistration &GetInstance();
  
  /**
   * @brief Cleans up and deletes the roster object list
   */
  ~MobRegistration();

  /**
   * @brief Creates a roster entry object, sets the deferred class type, and registers
   * @return MobMeta* to chain
   */
  template<class T>
  MobMeta* AddClass() {
    MobRegistration::MobMeta* info = new MobRegistration::MobMeta();
    info->SetMobClass<T>();
    Register(info);

    return info;
  }

  /**
   * @brief Gets the roster entry object at index
   * @param index position of the roster entry object
   * @return const MobMeta& The roster object
   */
  const MobMeta& At(int index);
  
  /**
   * @brief The size of the roster
   * @return const unsigned size
   */
  const unsigned Size();
  
  /**
   * @brief Used to register all mob and preview data
   * @param progress atomic thread safe counter for the successfully loaded assets
   */
  void LoadAllMobs(std::atomic<int>& progress);

};

/*! \brief Less typing */
#define MOBS MobRegistration::GetInstance()

/**
 * @brief Sets the deferred mob loader
 * @return MobMeta& to chain
 */
template<class T>
inline MobRegistration::MobMeta & MobRegistration::MobMeta::SetMobClass()
{
  loadMobClass = [this]() {
    if (mobFactory) {
      delete mobFactory;
      mobFactory = nullptr;
    }

    mobFactory = new T(new Field(6, 3));

    if (!placeholderTexture) {
      placeholderTexture = TEXTURES.LoadTextureFromFile(GetPlaceholderTexturePath());
    }
  };


  return *this;
}
