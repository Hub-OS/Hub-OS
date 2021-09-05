/*! \file bnCardPackageManager.h */

#pragma once

#include <SFML/Graphics.hpp>
#include "bnPackageManager.h"
#include "bnElements.h"
#include "bnCard.h"
#include "bindings/bnCardImpl.h"

/*! \brief Use this to register card mods with the engine
*/

class Character;
class CardAction;

class CardImpl {
public:
  virtual ~CardImpl() {};
  virtual std::shared_ptr<CardAction> BuildCardAction(std::shared_ptr<Character> user, Battle::Card::Properties& props) = 0;
};

struct CardMeta final : public PackageManager<CardMeta>::Meta<CardImpl> {
  Battle::Card::Properties properties;
  std::shared_ptr<sf::Texture> iconTexture; /*!< Icon used in hand */
  std::shared_ptr<sf::Texture> previewTexture; /*!< Picture used in select widget */
  std::vector<char> codes;

  /**
  * @brief
  */
  CardMeta();

  /**
   * @brief Delete
   */
  ~CardMeta();

  void OnMetaParsed() override;

  CardMeta& SetPreviewTexture(const std::shared_ptr<sf::Texture> texture);

  /**
   * @brief Device and face generally of the navi
   * @param icon texture
   * @return CardMeta& to chain
   */
  CardMeta& SetIconTexture(const std::shared_ptr<sf::Texture> icon);

  CardMeta& SetCodes(const std::vector<char> codes);

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

  // mutable to allow scripting to modify this
  Battle::Card::Properties& GetCardProperties();

  const std::vector<char> GetCodes() const;
};

class CardPackageManager : public PackageManager<CardMeta> {};