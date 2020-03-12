#pragma once

#include <string>
#include <tuple>
#include <map>
#include "bnElements.h"

using std::string;

class BattleScene;
class SelectedCardsUI;

/**
 * @class Card
 * @author mav
 * @date 05/05/19
 * @brief Describes card record entry in database of loaded cards
 * 
 * This will be expanded to load the corresponding script information
 * 
 */
class Card {
public:
  // TODO: take these out from Atk+10 quick hack
  friend class BattleScene;
  friend class SelectedCardsUI;

  /**
   * @brief Cards are not designed to have default or partial data. Must provide all at once.
   */
  Card(string uuid, char code, unsigned damage, Element element, string sname, string desc, string verboseDesc, unsigned rarity);
  
  /**
   * @brief copies card data
   */
  Card(const Card& copy);
  Card();
  ~Card();
  
  /**
   * @brief Get extra card description. Shows up on library.
   * @return string
   */
  const string GetVerboseDescription() const;
  
  /**
   * @brief Returns short description. shows up on card.
   * @return string
   */
  const string GetDescription() const;
  
  /**
   * @brief Name of card
   * @return string
   */
  const string GetShortName() const;
  
  /**
   * @brief Code of card
   * @return char
   */
  const char GetCode() const;
  
  /**
   * @brief Strength of card
   * @return unsigned
   */
  const unsigned GetDamage() const;

  
  /**
   * @brief Card ID of card
   * @return string
   */
  const string GetUUID() const;
  
  /**
   * @brief Element of card
   * @return Element
   */
  const Element GetElement() const;

  /**
  * @brief Secondary element of card 
  * @return Element
  *
  * @warning This element is used internally. If this is the same as the primary element, this element returns None
  */
  const Element GetSecondaryElement() const;
  
  /**
   * @brief Rarity of card
   * @return 0-5
   * @warning may be removed as rarity won't be used in the future
   */
  const unsigned GetRarity() const;

  /**
  * @brief Query if card summons one or more navis 
  * @returns true if flagged as a navi card, false otherwise
  */
  const bool IsNaviSummon() const;

  /**
* @brief Qeuery if card is tagged as a support card
* @returns true if flagged as a support card, false otherwise
*/
  const bool IsSupport() const;

  /**
* @brief Query if card should freeze time
* @returns true if flagged as a time freeze card, false otherwise
*/
  const bool IsTimeFreeze() const;

  /**
* @brief Query if card is tagged with user-defined information
* @returns true if flagged as input meta type, false otherwise
*/
  const bool IsTaggedAs(const std::string meta) const;

  /**
   * @brief Comparator for std map
   */
  struct Compare
  {
    bool operator()(const Card& lhs, const Card& rhs) const noexcept;
  };

  /**
   * @brief determine if cards are equal (by name and code) 
   */
  const bool operator==(const Card& rhs) const noexcept {
    return std::tie(shortname, code) == std::tie(rhs.shortname, rhs.code);
  }

  /**
 * @brief determine if this card is less than another (by name and code)
 */
  const bool operator<(const Card& rhs) const noexcept {
    return std::tie(shortname, code) < std::tie(rhs.shortname, rhs.code);
  }

  friend struct Compare;

private:
  std::string uuid;
  unsigned damage, unmodDamage;
  unsigned rarity; /*!< Todo: remove and add MB values*/
  char code;
  bool timeFreeze; /*!< Does this card rely on action items to resolve before resuming the battle scene? */
  bool navi; /*!< Does this card spawn navi(s)?*/
  bool support; /*!< Does this card heal or supply perks to the active navi and their cards?*/
  string shortname;
  string description;
  string verboseDescription;
  Element element, secondaryElement;
  std::map<std::string, std::string> metaTags; /*!< Cards can be tagged with additional user information*/
};
