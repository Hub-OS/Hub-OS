/*! \file bnNaviRegistration.h */

#pragma once

#include <map>
#include <vector>
#include <functional>
#include <atomic>
#include <SFML/Graphics.hpp>
#include "bnElements.h"
#include "bnPlayer.h"
#include "stx/result.h"
#include "stx/tuple.h"

class Player; // forward decl

/*! \brief Use this singleton to register custom navis and have them automatically appear on the select, overworld, and battle scenes
*/
class NaviRegistration {
public:

  /*! \brief Navi roster info object. Assign navi details to display with this. */
  class NaviMeta {
    friend class NaviRegistration;

    Player* navi; /*!< The net navi to construct */
    std::string packageId; /*!< Reverse domain name (reverse-DNS) identifier */
    std::string filepath; /*!< Package file path (optional field) */
    std::string special; /*!< The net navi's special description */
    std::string overworldAnimationPath; /*!< The net navi's overworld animation */
    std::string overworldTexturePath; /*!< The path of the texture to load */
    std::string mugshotTexturePath;
    std::string mugshotAnimationPath;
    std::string emotionsTexturePath; /*!< The path of the texture containing all possible emotions, ordered by emotion enum */
    std::string name; /*!< The net navi's name */
    std::shared_ptr<sf::Texture> iconTexture; /*!< Icon on the top of the screen */
    std::shared_ptr<sf::Texture> previewTexture; /*!< Roster profile picture */
    unsigned atk; /*!< Attack level of the net navi */
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
    template<class T, typename... Args> NaviMeta& SetNaviClass(Args&&...);
   
    /**
    * @brief Sets an identifier for this package in the engine
    * @param Reverse-DNS identifier
    * @return NaviMeta& to chain
    */
    NaviMeta& SetPackageID(const std::string& id);

    /**
     * @brief Sets the filepath associated with this navi meta data
     * @return NaviMeta& to chain
     */
    NaviMeta& SetFilePath(const std::string& filepath);

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
    NaviMeta& SetAttack(const unsigned atk);
    
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
   * @brief Sets the mugshot animation path used in textboxes
   * @return NaviMeta& to chain
   */
    NaviMeta& SetMugshotAnimationPath(const std::string& path);

    /**
     * @brief Sets the texture of the mugshot animation
     * @param texture
     * @return NaviMeta& to chain
     */
    NaviMeta& SetMugshotTexturePath(const std::string& texture);

    /**
     * @brief Sets the emotions texture atlas
     * @param texture
     * @return NaviMeta& to chain
     */
    NaviMeta& SetEmotionsTexturePath(const std::string& texture);

    /**
     * @brief Sets the overworld animation path used in menu screen
     * @return NaviMeta& to chain
     */
    NaviMeta& SetOverworldAnimationPath(const std::string& path);
    
    /**
     * @brief Sets the texture of the overworld animation
     * @param texture
     * @return NaviMeta& to chain
     */
    NaviMeta& SetOverworldTexturePath(const std::string& texture);
    
    /**
     * @brief Sets the texture of the preview used in select screen
     * @param texture
     * @return NaviMeta& to chain
     */
    NaviMeta& SetPreviewTexture(const std::shared_ptr<Texture> texture);
    
    /**
     * @brief Device and face generally of the navi
     * @param icon texture
     * @return NaviMeta& to chain
     */
    NaviMeta& SetIconTexture(const std::shared_ptr<sf::Texture> icon);

    /**
    * @brief Gets the package identifier
    * @return const std::string&
    */
    const std::string& GetPackageID() const;

    /**
    * @brief Gets the package file path, if supplied
    * @return const std::string&
    */
    const std::string& GetFilePath() const;

    /**
     * @brief Gets the icon texture to draw
     * @return const sf::Texture&
     */
    const std::shared_ptr<sf::Texture> GetIconTexture() const;

    /**
     * @brief Gets the overworld texture path to load
     * @return const std::string&
     */
    const std::string& GetOverworldTexturePath() const;
    
    /**
     * @brief Gets the overworld animation path
     * @return const std::string&
     */
    const std::string& GetOverworldAnimationPath() const;

    /**
   * @brief Gets the mugshot texture path
   * @return const std::string&
   */
    const std::string& GetMugshotTexturePath() const;

    /**
   * @brief Gets the mugshot animation path
   * @return const std::string&
   */
    const std::string& GetMugshotAnimationPath() const;

    /**
   * @brief Gets the emotions texture path
   * @return const std::string&
   */
    const std::string& GetEmotionsTexturePath() const;

    /**
     * @brief Gets the preview texture to draw
     * @return const sf::Texture&
     */
    const std::shared_ptr<sf::Texture> GetPreviewTexture() const;
    
    /**
     * @brief Gets the net navi name
     * @return const std::string (copy)
     */
    const std::string GetName() const;
    
    /**
     * @brief Gets the navi HP
     * @return int
     */
    int GetHP() const;
    
    /**
     * @brief Gets the navi HP as a string to display
     * @return const std::string (copy)
     */
    const std::string GetHPString() const;
    
    /**
     * @brief Gets the navi speed as a string to display
     * @return const std::string (copy)
     */
    const std::string GetSpeedString() const;
    
    /**
     * @brief Get the attack strength as string to display
     * @return const std::string (copy)
     */
    const std::string GetAttackString() const;
    
    /**
     * @brief Get the special description string to display
     * @return const std::string
     */
    const std::string& GetSpecialDescriptionString() const;
    
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
  std::map<std::string, NaviMeta*> roster; /*!< Complete roster of net navis to load */
  
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
 * @brief Registers a navi through a NaviMeta data object
 * @param info
 * @return if info was able to commit or not
 */
  stx::result_t<bool> Commit(NaviMeta* info);

  /**
   * @brief Creates a navi roster entry and sets the deffered navi loader for navi type T
   * @return NaviMeta* roster data object
   */
  template<class T, typename... Args>
  NaviMeta* AddClass(Args&&... args) {
    NaviRegistration::NaviMeta* info = new NaviRegistration::NaviMeta();
    info->SetNaviClass<T>(std::forward<decltype(args)>(args)...);
    return info;
  }

  /**
   * @brief Get the navi info entry by package identifier 
   * @param id package reverse-DNS identifier
   * @return NaviMeta& of navi entry
   * @throws std::runtime_error if not found
   */
  NaviMeta& FindByPackageID(const std::string& id);
  stx::result_t<std::string> RemovePackageByID(const std::string& id);

  bool HasPackage(const std::string& id);
  const std::string FirstValidPackage();
  const std::string GetPackageBefore(const std::string& id);
  const std::string GetPackageAfter(const std::string& id);
  stx::result_t<std::string> GetPackageFilePath(const std::string& id);
  
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
  
  stx::result_t<bool> LoadNaviFromPackage(const std::string& path);
  stx::result_t<bool> LoadNaviFromZip(const std::string& path);
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
template<class T, typename... Args>
inline NaviRegistration::NaviMeta & NaviRegistration::NaviMeta::SetNaviClass(Args&&... args)
{
  loadNaviClass = [this, args = std::make_tuple(std::forward<decltype(args)>(args)...)]() mutable {
    navi = stx::make_ptr_from_tuple<T>(args);
    hp = navi->GetHealth();
  };

  return *this;
}
