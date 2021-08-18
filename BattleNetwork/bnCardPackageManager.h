/*! \file bnCardPackageManager.h */

#pragma once

#include <SFML/Graphics.hpp>
#include "bnPackageManager.h"
#include "bnElements.h"
#include "bnCard.h"

/*! \brief Use this to register card mods with the engine
*/

class Character;
class CardAction;

class CardImpl {
public:
  virtual ~CardImpl() {};
  virtual CardAction* BuildCardAction(Character* user, Battle::Card::Properties& props) = 0;
};

struct CardMeta final : public PackageManager<CardMeta>::Meta<CardImpl> {
  Battle::Card::Properties properties;
  std::shared_ptr<sf::Texture> iconTexture; /*!< Icon used in hand */
  std::shared_ptr<sf::Texture> previewTexture; /*!< Picture used in select widget */

  /**
  * @brief
  */
  CardMeta();

  /**
   * @brief Delete
   */
  ~CardMeta();

  void OnMetaConfigured() override;

  CardMeta& SetPreviewTexture(const std::shared_ptr<sf::Texture> texture);

  /**
   * @brief Device and face generally of the navi
   * @param icon texture
   * @return CardMeta& to chain
   */
  CardMeta& SetIconTexture(const std::shared_ptr<sf::Texture> icon);

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
};

class CardPackageManager : public PackageManager<CardMeta> {};