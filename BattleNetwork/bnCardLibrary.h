#pragma once
#include "bnCard.h"
#include <set>
#include <list>

using std::multiset;

/**
 * @class CardLibrary
 * @author mav
 * @date 05/05/19
 * @brief Loads all cards from the database and adds new cards from in-game
 * 
 * @important will send cards to be parsed before storing them in the list
 * 
 * Provides utilities to ensure cards are valid (existing) before using them
 * in-battle. 
 * 
 * Acts as the player's card-pool
 */
class CardLibrary {
public:
  typedef multiset<Battle::Card, Battle::Card::Compare>::iterator Iter;

  /**
   * @brief Invokes LoadLibrary()
   */
  CardLibrary();
  ~CardLibrary();
  
  /**
   * @brief If first time calling, creates the singleton instance
   * @return CardLibrary&
   */
  static CardLibrary & GetInstance();

  /**
   * @brief list.begin()
   * @return list<Card>::iterator
   */
  Iter Begin();
  
  /**
   * @brief list.end()
   * @return list<Card>::iterator
   */
  Iter End();
  
  /**
   * @brief Count of number of loaded cards
   * @return const unsigned
   */
  const unsigned GetSize() const;

  /**
   * @brief Adds a card directly into the library
   * @param card entry to add to list
   */
  void AddCard(Battle::Card card);
  
  /**
   * @brief Checks if a card with the same code exists in the library
   * @param card to compare
   * @return true if both name and code match any cards in the library
   */
  bool IsCardValid(Battle::Card& card);

  /**
   * @brief Given a card name, return all existing codes from the pool
   * @param card to compare
   * @return list<char> codes
   */
  std::list<char> GetCardCodes(const Battle::Card& card);

  /**
   * @brief Returns number of copies in this data pool
   * @return integer number of copies or zero if not found
   */
  const int GetCountOf(const Battle::Card& card);
  
  /**
   * @brief Given a name and code, return the card from the database
   * @param name of the card
   * @param code of the card
   * @return Copy of entry. If no entry was found, a card with error information is returned
   */
  Battle::Card GetCardEntry(const std::string name, const char code);

  /**
  * @brief Writes library to disc
  * @param path path to output file.
  * @warning will overwrite if path is already existing!
  * @return true if successful, false otherwise
  *
  * Writes the library as-is at the time of writing
  */
  const bool SaveLibrary(const std::string& path);

protected:
 /**
  * @brief Reads in libary file and parses card data
  * @important Will also parse card scripts in the future before adding
  * @param path to library database
  *
  * If no card data is found or if no data is successfully read, library will be empty
  *
  * Reads card entry data and tries to add cards. It avoids duplicate entries. Cards
  * with the same name but different code do not count as duplicates. This allows the pool
  * to recognize and expect cards with particular card codes.
  */
  void LoadLibrary(const std::string& path);

private:
  mutable multiset<Battle::Card, Battle::Card::Compare> library; /*!< the card pool used by all card resources */
};

#define CHIPLIB CardLibrary::GetInstance()

