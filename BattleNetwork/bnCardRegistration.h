/*! \file bnCardRegistration.h */

#pragma once

#include <map>
#include <vector>
#include <functional>
#include <atomic>
#include <SFML/Graphics.hpp>
#include "bnElements.h"
#include "bnCard.h"
#include "stx/result.h"
#include "stx/tuple.h"

/*! \brief Use this to register card mods with the engine
*/

class Character;
class CardAction;

class CardImpl {
public:
  virtual ~CardImpl() {};
  virtual CardAction* BuildCardAction(Character* user, Battle::Card::Properties& props) = 0;
};

class CardRegistration {
public:

  /*! \brief Navi roster info object. Assign navi details to display with this. */
  class CardMeta {
    friend class CardRegistration;

    CardImpl* card{ nullptr };
    Battle::Card::Properties properties;
    std::string packageId; /*!< Reverse domain name (reverse-DNS) identifier */
    std::string filepath;
    std::shared_ptr<sf::Texture> iconTexture; /*!< Icon used in hand */
    std::shared_ptr<sf::Texture> previewTexture; /*!< Picture used in select widget */
    std::function<void()> loadClass; /*!< Deffered loading. Only load class when needed */

  public:
    /**
     * @brief
     */
    CardMeta();

    /**
     * @brief Delete
     */
    ~CardMeta();

    /**
     * @brief Prepares the loadClass deffered loading function
     * @return NaviMeta& to chain
     */
    template<class T, typename... Args> CardMeta& SetClass(Args&&...);

    /**
    * @brief Sets an identifier for this package in the engine
    * @param Reverse-DNS identifier
    * @return CardMeta& to chain
    */
    CardMeta& SetPackageID(const std::string& id);

    /**
     * @brief Sets the filepath associated with this meta data
     * @return CardMeta& to chain
     */
    CardMeta& SetFilePath(const std::string& filepath);


    CardMeta& SetPreviewTexture(const std::shared_ptr<sf::Texture> texture);

    /**
     * @brief Device and face generally of the navi
     * @param icon texture
     * @return CardMeta& to chain
     */
    CardMeta& SetIconTexture(const std::shared_ptr<sf::Texture> icon);

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
     * @brief Gets the preview texture to draw
     * @return const sf::Texture&
     */
    const std::shared_ptr<sf::Texture> GetPreviewTexture() const;

    Battle::Card::Properties& GetCardProperties();

    /**
     * @brief If not constructed, builds the card using the deffered loader
     * @return CardImpl*
     */
    CardImpl* GetCard();
  };

private:
  std::map<std::string, CardMeta*> roster; /*!< Complete roster of cards to load */

public:
  /**
   * @brief Deletes and cleansup roster data
   */
  ~CardRegistration();

  /**
 * @brief Registers a card through a CardMeta data object
 * @param info
 * @return if info was able to commit or not
 */
  stx::result_t<bool> Commit(CardMeta* info);

  /**
   * @brief Creates a card roster entry and sets the deffered class loader for type T
   * @return CardMeta* roster data object
   */
  template<class T, typename... Args>
  CardMeta* AddClass(Args&&... args) {
    CardRegistration::CardMeta* info = new CardRegistration::CardMeta();
    info->SetClass<T>(std::forward<decltype(args)>(args)...);
    return info;
  }

  /**
   * @brief Get the card info entry by package identifier
   * @param id package reverse-DNS identifier
   * @return CardMeta& of card entry
   * @throws std::runtime_error if not found
   */
  CardMeta& FindByPackageID(const std::string& id);
  stx::result_t<std::string> RemovePackageByID(const std::string& id);

  bool HasPackage(const std::string& id);
  const std::string FirstValidPackage();
  const std::string GetPackageBefore(const std::string& id);
  const std::string GetPackageAfter(const std::string& id);
  stx::result_t<std::string> GetPackageFilePath(const std::string& id);

  /**
   * @brief Get the size of the card roster
   * @return const unsigned size
   */
  const unsigned Size();

  /**
   * @brief Used at startup, loads every card queued by the roster
   * @param progress atomic thread safe counter when loading resources
   */
  void LoadAllCards(std::atomic<int>& progress);

  stx::result_t<bool> LoadCardFromPackage(const std::string& path);
  stx::result_t<bool> LoadCardFromZip(const std::string& path);
};

/**
 * @brief Sets the deferred type loader T
 *
 * Automatically sets battle texture and overworld texture from the card class
 * Automatically sets health from card class
 *
 * @return CardMeta& object for chaining
 */
template<class T, typename... Args>
inline CardRegistration::CardMeta& CardRegistration::CardMeta::SetClass(Args&&... args)
{
  loadClass = [this, args = std::make_tuple(std::forward<decltype(args)>(args)...)]() mutable {
    card = stx::make_ptr_from_tuple<T>(args);
  };

  return *this;
}
