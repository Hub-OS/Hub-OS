#pragma once

#include <string>
#include "bnElements.h"

using std::string;

/**
 * @class Chip
 * @author mav
 * @date 05/05/19
 * @brief Describes chip record entry in database of loaded chips
 * 
 * This will be expanded to load the corresponding script information
 * 
 * TODO: Add support chip flag information
 * TODO: Add Time Freeze attribute
 */
class Chip {
public:
  /**
   * @brief Chips are not designed to have default or partial data. Must provide all at once.
   */
  Chip(unsigned id, unsigned icon, char code, unsigned damage, Element element, string sname, string desc, string verboseDesc, unsigned rarity);
  
  /**
   * @brief copies chip data
   */
  Chip(const Chip& copy);
  Chip();
  ~Chip();
  
  /**
   * @brief Get extra chip description. Shows up on library.
   * @return string
   */
  const string GetVerboseDescription() const;
  
  /**
   * @brief Returns short description. shows up on chip.
   * @return string
   */
  const string GetDescription() const;
  
  /**
   * @brief Name of chip
   * @return string
   */
  const string GetShortName() const;
  
  /**
   * @brief Code of chip
   * @return char
   */
  const char GetCode() const;
  
  /**
   * @brief Strength of chip
   * @return unsigned
   */
  const unsigned GetDamage() const;
  
  /**
   * @brief Battle mini icon ID of chip
   * @return unsigned
   */
  const unsigned GetIconID() const;
  
  /**
   * @brief Card ID of chip
   * @return unsigned
   */
  const unsigned GetID() const;
  
  /**
   * @brief Element of chip
   * @return Element
   */
  const Element GetElement() const;
  
  /**
   * @brief Rarity of chip
   * @return 0-5
   * @warning may be removed as rarity won't be used in the future
   */
  const unsigned GetRarity() const;

  /**
   * @brief Comparator for std map
   */
  struct Compare
  {
    bool operator()(const Chip& lhs, const Chip& rhs) const noexcept;
  };

  /**
   * @brief determine if chips are equal (by name and code) 
   */
  const bool operator==(const Chip& rhs) const noexcept {
    return std::tie(shortname, code) == std::tie(rhs.shortname, rhs.code);
  }

  /**
 * @brief determine if this chip is less than another (by name and code)
 */
  const bool operator<(const Chip& rhs) const noexcept {
    return std::tie(shortname, code) < std::tie(rhs.shortname, rhs.code);
  }

  friend struct Compare;

private:
  unsigned ID;
  unsigned icon;
  unsigned damage;
  unsigned rarity;
  char code;
  string shortname;
  string description;
  string verboseDescription;
  Element element;
};