#pragma once

#include <string>
#include <tuple>
#include <map>
#include "bnElements.h"

using std::string;

class BattleScene;
class SelectedChipsUI;

/**
 * @class Chip
 * @author mav
 * @date 05/05/19
 * @brief Describes chip record entry in database of loaded chips
 * 
 * This will be expanded to load the corresponding script information
 * 
 */
class Chip {
public:
  // TODO: take these out from Atk+10 quick hack
  friend class BattleScene;
  friend class SelectedChipsUI;

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
  * @brief Secondary element of chip 
  * @return Element
  *
  * @warning This element is used internally. If this is the same as the primary element, this element returns None
  */
  const Element GetSecondaryElement() const;
  
  /**
   * @brief Rarity of chip
   * @return 0-5
   * @warning may be removed as rarity won't be used in the future
   */
  const unsigned GetRarity() const;

  /**
  * @brief Query if chip summons one or more navis 
  * @returns true if flagged as a navi chip, false otherwise
  */
  const bool IsNaviSummon() const;

  /**
* @brief Qeuery if chip is tagged as a support chip
* @returns true if flagged as a support chip, false otherwise
*/
  const bool IsSupport() const;

  /**
* @brief Query if chip should freeze time
* @returns true if flagged as a time freeze chip, false otherwise
*/
  const bool IsTimeFreeze() const;

  /**
* @brief Query if chip is tagged with user-defined information
* @returns true if flagged as input meta type, false otherwise
*/
  const bool IsTaggedAs(const std::string meta) const;

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
  unsigned damage, unmodDamage;
  unsigned rarity; /*!< Todo: remove and add MB values*/
  char code;
  bool timeFreeze; /*!< Does this chip rely on action items to resolve before resuming the battle scene? */
  bool navi; /*!< Does this chip spawn navi(s)?*/
  bool support; /*!< Does this chip heal or supply perks to the active navi and their chips?*/
  string shortname;
  string description;
  string verboseDescription;
  Element element, secondaryElement;
  std::map<std::string, std::string> metaTags; /*!< Chips can be tagged with additional user information*/
};
