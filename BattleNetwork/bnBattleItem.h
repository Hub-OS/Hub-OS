#pragma once

#include <string>
#include "bnCard.h"
// #include "bnHealthItem.h"
// #include "bnZennyItem.h"

/**
 * @class BattleItem
 * @author mav
 * @date 13/05/19
 * @brief BattleItem is a container for the awarded object type and can be money, a card, or hp gain.
 * 
 * @warning Must be refactored to support scripted card types
 * @important Like a union struct, can be any type. You must check IsCard(), IsHP(), or IsZenny() respectively.
 */
class BattleItem {
private:
  std::string cardUUID; /*!< The ID for the card card */
  std::string name; /*!< Name of the item or card */
  bool isCard; /*!< Flag if card */
  bool isZenny; /*!< Flag if zenny */
  bool isHP; /*!< Flag if HP */
  Card card; /*!< Card data */
  // HealthItem hpGain; // +10, +50, +100, +200 HP gained post battle
  // ZennyItem zennyGain; // stores arbitrary zenny amount
public:
  /**
   * @brief Constructs a battle item with card data 
   * @param card the card to copy and reward player with
   */
  BattleItem(Card card) : card(card), name(card.GetShortName()), cardUUID(card.GetUUID()) { isCard = true; isZenny = isHP = false; }
  BattleItem(const BattleItem& rhs) : card(rhs.card) { isCard = rhs.isCard; isZenny = rhs.isZenny; isHP = rhs.isHP; cardUUID = rhs.cardUUID; name = rhs.name; }
  
  /**
   * @brief Get card UUID
   * @return string
   */
  std::string GetUUID() { return cardUUID; }
  
  /**
   * @brief Get item or card name
   * @return const string
   */
  const std::string GetName() { return name;  }
  
  /**
   * @brief Query if card
   * @return true if battle item is a card and contains card data
   */
  bool IsCard() { return isCard; }
  
  /**
   * @brief Get card data
   * @return Card
   */
  Card GetCard() { return card; }
  
  /**
   * @brief Get card code 
   * @return char
   */
  char GetCardCode() {
    return card.GetCode();
  }
};