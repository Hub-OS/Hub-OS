#pragma once

#include <string>
#include "bnChip.h"

/**
 * @class BattleItem
 * @author mav
 * @date 13/05/19
 * @brief BattleItem is a container for the awarded object type and can be money, a chip, or extra life.
 * 
 * @warning Must be refactored to support scripted chip types
 * @important Like a union struct, can be any type. You must check IsChip(), IsHP(), or IsZenny() respectively.
 */
class BattleItem {
private:
  int cardID; /*!< The ID for the chip card */
  std::string name; /*!< Name of the item or chip */
  bool isChip; /*!< Flag if chip */
  bool isZenny; /*!< Flag if money */
  bool isHP; /*!< Flag if HP */
  Chip chip; /*!< Chip data */
public:
<<<<<<< HEAD
  BattleItem(std::string name, int id) : name(name), cardID(id), chip(0, 0, 0, 0, Element::NONE, "null", "null", "", 0) { isChip = isZenny = isHP = false; }
=======
  /**
   * @brief Constructs a battle item with chip data 
   * @param chip the chip to copy and reward player with
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  BattleItem(Chip chip) : chip(chip), name(chip.GetShortName()), cardID(chip.GetID()) { isChip = true; isZenny = isHP = false; }
  BattleItem(const BattleItem& rhs) : chip(rhs.chip) { isChip = rhs.isChip; isZenny = rhs.isZenny; isHP = rhs.isHP; cardID = rhs.cardID; name = rhs.name; }
  
  /**
   * @brief Get chip ID
   * @return int
   */
  int GetID() { return cardID; }
  
  /**
   * @brief Get item or chip name
   * @return const string
   */
  const std::string GetName() { return name;  }
  
  /**
   * @brief Query if chip
   * @return true if battle item is a chip and contains chip data
   */
  bool IsChip() { return isChip; }
<<<<<<< HEAD
  Chip GetChip() { return chip; }
=======
  
  /**
   * @brief Get chip data
   * @return Chip
   */
  Chip GetChip() { return chip; }
  
  /**
   * @brief Get chip code 
   * @return char
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  char GetChipCode() {
    return chip.GetCode();
  }
};