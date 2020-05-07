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
  class NaviMeta {
    friend class NaviRegistration;

    Player* navi; /*!< The net navi to construct */
    sf::Sprite symbol; /*!< The net navi's symbol */
    std::string special; /*!< The net navi's special description */
    std::string overworldAnimationPath; /*!< The net navi's overworld animation */
    std::string name; /*!< The net navi's name */
    std::shared_ptr<sf::Texture> overworldTexture; /*!< Texture of overworld animation */
    std::shared_ptr<sf::Texture> previewTexture; /*!< Roster profile picture */
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
    NaviMeta();
    
    /**
     * @brief Delete navi pointer
     */
    ~NaviMeta();

    /**
     * @brief Prepares the loadNaviClass deffered loading function
     * @return NaviMeta& to chain
     */
    template<class T> NaviMeta& SetNaviClass();
    
    /**
     * @brief Set symbol texture and cropped at 15x15 dimensions
     * @param symbol texture
     * @return NaviMeta& to chain
     */
    NaviMeta& SetSymbolTexture(sf::Texture& symbol);
    
    /**
     * @brief Sets special description information of the navi
     * @return NaviMeta& to chain
     */
    NaviMeta& SetSpecialDescription(const std::string&& special);
    
    /**
     * @brief Sets attack level to display
     * @param atk attack strength
     * @return NaviMeta& to chain
     */
    NaviMeta& SetAttack(const int atk);
    
    /**
     * @brief Sets charged attack level to display
     * @param atk charge attack strength
     * @return NaviMeta& to chain
     */
    NaviMeta& SetChargedAttack(const int atk);
    
    /**
     * @brief Sets speed level to display
     * @param speed how fast the navi is expected to be
     * @return NaviMeta& to chain
     */
    NaviMeta& SetSpeed(const double speed);
    
    /**
     * @brief Sets the health to display
     * @param HP health
     * @return NaviMeta& to chain
     */
    NaviMeta& SetHP(const int HP);
    
    /**
     * @brief Toggles if a sword based navi or buster
     * @param enabled true if sword, default is false
     * @return NaviMeta& to chain
     */
    NaviMeta& SetIsSword(const bool enabled);
    
    /**
     * @brief Sets the overworld animation path used in menu screen
     * @return NaviMeta& to chain
     */
    NaviMeta& SetOverworldAnimationPath(const std::string&& path);
    
    /**
     * @brief Sets the texture of the overworld animation
     * @param texture
     * @return NaviMeta& to chain
     */
    NaviMeta& SetOverworldTexture(const std::shared_ptr<sf::Texture> texture);
    
    /**
     * @brief Sets the texture of the preview used in select screen
     * @param texture
     * @return NaviMeta& to chain
     */
    NaviMeta& SetPreviewTexture(const std::shared_ptr<Texture> texture);
    
    /**
     * @brief Gets the overworld texture to draw
     * @return const sf::Texture&
     */
    const std::shared_ptr<Texture> GetOverworldTexture() const;
    
    /**
     * @brief Gets the overworld animation path
     * @return const std::string&
     */
    const std::string& GetOverworldAnimationPath() const;
    
    /**
     * @brief Gets the preview texture to draw
     * @return const sf::Texture&
     */
    const std::shared_ptr<sf::Texture> GetPreviewTexture() const;
    
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

    /**
     * @brief If not constructed, builds the navi using the deffered loader
     * @return Player*
     */
    Player* GetNavi();
  };

private:
  std::vector<NaviMeta*> roster; /*!< Complete roster of net navis to load */
 
  /**
   * @brief Registers a navi through a NaviMeta data object
   * @param info
   */
  void Register(NaviMeta* info);
  
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
   * @return NaviMeta* roster data object
   */
  template<class T>
  NaviMeta* AddClass() {
    NaviRegistration::NaviMeta* info = new NaviRegistration::NaviMeta();
    info->SetNaviClass<T>();
    Register(info);

    return info;
  }

  /**
   * @brief Get the navi info entry at roster index 
   * @param index roster index
   * @return NaviMeta& of navi entry
   * @throws std::runtime_error if index is greater than number of entries or less than zero
   */
  NaviMeta& At(int index);
  
  /**
   * @brief Get the size of the navi roster
   * @return const unsigned size
   */
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
 * @return NaviMeta& object for chaining
 */
template<class T>
inline NaviRegistration::NaviMeta & NaviRegistration::NaviMeta::SetNaviClass()
{
  loadNaviClass = [this]() {
    // NOTE: This used to extract information from the class type T itself
    //        and would mofidy the preview in the navi select screen.
    //        Lots has changed since then and this may be useless.
    navi = new T(); 
    SetOverworldTexture(navi->getTexture()); // TODO: Overworld doesn't need to be stored in the battle object
    hp = navi->GetHealth();
  };

  return *this;
}
