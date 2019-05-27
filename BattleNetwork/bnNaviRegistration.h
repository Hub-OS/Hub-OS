/*! \file bnNaviRegistration.h */

#pragma once

#include <map>
#include <vector>
#include <functional>
#include <atomic>
#include <SFML/Graphics.hpp>
#include "bnElements.h"
#include "bnPlayer.h"

class Player; // forward decl

typedef int SelectedNavi;

/*! \brief Use this singleton to register custom navis and have them automatically appear on the select, overworld, and battle scenes
*/
class NaviRegistration {
public:

  /*! \brief Navi roster info object. Assign navi details to display with this. */
  class NaviInfo {
    friend class NaviRegistration;

    Player* navi; /*!< The net navi to construct */
    sf::Sprite symbol; /*!< The net navi's symbol */
    std::string special; /*!< The net navi's special description */
    std::string overworldAnimationPath; /*!< The net navi's overworld animation */
    std::string battleAnimationPath; /*!< The net navi's battle animation */
    std::string name; /*!< The net navi's name */
    sf::Texture* overworldTexture; /*!< Texture of overworld animation */
    sf::Texture* battleTexture; /*!< Texture of the battle animation */
    int atk; /*!< Attack level of the net navi */
    int chargedAtk; /*!< Charged attack level of the net navi */
    double speed; /*!< The speed of the navi */
    int hp; /*!< The health of the navi */
    bool isSword; /*!< Is buster or sword based navi */

    std::function<void()> loadNaviClass; /*!< Deffered navi loading. Only load navi class when needed */

    public:
    /**
     * @brief Sets every number to 1 and string to "None"
     */
    NaviInfo();
    
    /**
     * @brief Delete navi pointer
     */
    ~NaviInfo();

    /**
     * @brief Prepares the loadNaviClass deffered loading function
     * @return NaviInfo& to chain
     */
    template<class T> NaviInfo& SetNaviClass();
    
    /**
     * @brief Set symbol texture and cropped at 15x15 dimensions
     * @param symbol texture
     * @return NaviInfo& to chain
     */
    NaviInfo& SetSymbolTexture(sf::Texture& symbol);
    
    /**
     * @brief Sets special description information of the navi
     * @return NaviInfo& to chain
     */
    NaviInfo& SetSpecialDescription(const std::string&& special);
    
    /**
     * @brief Sets attack level to display
     * @param atk attack strength
     * @return NaviInfo& to chain
     */
    NaviInfo& SetAttack(const int atk);
    
    /**
     * @brief Sets charged attack level to display
     * @param atk charge attack strength
     * @return NaviInfo& to chain
     */
    NaviInfo& SetChargedAttack(const int atk);
    
    /**
     * @brief Sets speed level to display
     * @param speed how fast the navi is expected to be
     * @return NaviInfo& to chain
     */
    NaviInfo& SetSpeed(const double speed);
    
    /**
     * @brief Sets the health to display
     * @param HP health
     * @return NaviInfo& to chain
     */
    NaviInfo& SetHP(const int HP);
    
    /**
     * @brief Toggles if a sword based navi or buster
     * @param enabled true if sword, default is false
     * @return NaviInfo& to chain
     */
    NaviInfo& SetIsSword(const bool enabled);
    
    /**
     * @brief Sets the overworld animation path used in menu screen
     * @return NaviInfo& to chain
     */
    NaviInfo& SetOverworldAnimationPath(const std::string&& path);
    
    /**
     * @brief Sets the texture of the overworld animation
     * @param texture
     * @return NaviInfo& to chain
     */
    NaviInfo& SetOverworldTexture(const sf::Texture* texture);
    
    /**
     * @brief Sets the battle animation path used in menu screen
     * @return NaviInfo& to chain
     */
    NaviInfo& SetBattleAnimationPath(const std::string&& path);
    
    /**
     * @brief Sets the texture of the battle animation used in select screen
     * @param texture
     * @return NaviInfo& to chain
     */
    NaviInfo& SetBattleTexture(const sf::Texture* texture);
    
    /**
     * @brief Gets the overworld texture to draw
     * @return const sf::Texture&
     */
    const sf::Texture& GetOverworldTexture() const;
    
    /**
     * @brief Gets the overworld animation path
     * @return const std::string&
     */
    const std::string& GetOverworldAnimationPath() const;
    
    /**
     * @brief Gets the battle texture to draw
     * @return const sf::Texture&
     */
    const sf::Texture& GetBattleTexture() const;
    
    /**
     * @brief Gets the battle animation path
     * @return const std::string&
     */
    const std::string& GetBattleAnimationPath() const;
    
    /**
     * @brief Gets the net navi name
     * @return const std::string
     */
    const std::string GetName() const;
    
    /**
     * @brief Gets the navi HP as a string to display
     * @return const std::string
     */
    const std::string GetHPString() const;
    
    /**
     * @brief Gets the navi speed as a string to display
     * @return const std::string
     */
    const std::string GetSpeedString() const;
    
    /**
     * @brief Get the attack strength as string to display
     * @return const std::string
     */
    const std::string GetAttackString() const;
    
    /**
     * @brief Get the special description string to display
     * @return const std::string
     */
    const std::string GetSpecialDescriptionString() const;
    
    /**
     * @brief Gets the element of the navi
     * @return Element
     */
    const Element GetElement() const;

<<<<<<< HEAD
=======
    /**
     * @brief If not constructed, builds the navi using the deffered loader
     * @return Player*
     */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
    Player* GetNavi();
  };

private:
<<<<<<< HEAD
  std::vector<NaviInfo*> roster;

=======
  std::vector<NaviInfo*> roster; /*!< Complete roster of net navis to load */
 
  /**
   * @brief Registers a navi through a NaviInfo data object
   * @param info
   */
  void Register(NaviInfo* info);
  
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
public:
  /**
   * @brief If first call, inits the singleton. Returns the resource.
   * @return  NaviRegistration&
   */
  static NaviRegistration& GetInstance();
  
  /**
   * @brief Deletes and cleansup roster data
   */
  ~NaviRegistration(); 

  /**
   * @brief Creates a navi roster entry and sets the deffered navi loader for navi type T
   * @return NaviInfo* roster data object
   */
  template<class T>
  NaviInfo* AddClass() {
    NaviRegistration::NaviInfo* info = new NaviRegistration::NaviInfo();
    info->SetNaviClass<T>();
    this->Register(info);

    return info;
  }

<<<<<<< HEAD
  void Register(NaviInfo* info);
  NaviInfo& At(int index);
=======
  /**
   * @brief Get the navi info entry at roster index 
   * @param index roster index
   * @return NaviInfo& of navi entry
   * @throws std::runtime_error if index is greater than number of entries or less than zero
   */
  NaviInfo& At(int index);
  
  /**
   * @brief Get the size of the navi roster
   * @return const unsigned size
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  const unsigned Size();
  
  /**
   * @brief Used at startup, loads every navi queued by the roster
   * @param progress atomic thread safe counter when loading resources
   */
  void LoadAllNavis(std::atomic<int>& progress);
  
};

/*! \brief Shorthand for grabbing resource instance */
#define NAVIS NaviRegistration::GetInstance()

/**
 * @brief Sets the deferred type loader T
 * 
 * Automatically sets battle texture and overworld texture from the net navi class
 * Automatically sets health from net navi class
 * 
 * @return NaviInfo& object for chaining
 */
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
