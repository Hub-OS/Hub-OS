#pragma once
#include "bnChip.h"
#include <list>

using std::list;

/**
 * @class ChipLibrary
 * @author mav
 * @date 05/05/19
 * @brief Loads all chips from the database and adds new chips from in-game
 * 
 * @important will send chips to be parsed before storing them in the list
 * 
 * Provides utilities to ensure chips are valid (existing) before using them
 * in-battle. 
 * 
 * Acts as the player's chip-pool
 */
class ChipLibrary {
public:
  typedef list<Chip>::iterator Iter;

  /**
   * @brief Invokes LoadLibrary()
   */
  ChipLibrary();
  ~ChipLibrary();
  
  /**
   * @brief If first time calling, creates the singleton instance
   * @return ChipLibrary&
   */
  static ChipLibrary & GetInstance();

  /**
   * @brief list.begin()
   * @return list<Chip>::iterator
   */
  Iter Begin();
  
  /**
   * @brief list.end()
   * @return list<Chip>::iterator
   */
  Iter End();
  
  /**
   * @brief Count of number of loaded chips
   * @return const unsigned
   */
  const unsigned GetSize() const;

  /**
   * @brief Given input string, returns Element enum equivalent
   * @return Element::NONE if non matching, otherwise returns enum of same name
   */
  static const Element GetElementFromStr(const std::string);

  /**
   * @brief Given input enum element, returns string equivalent
   * @return "None" if non matching, otherwise returns string of enum name
   */
  static const std::string GetStrFromElement(const Element);

  /**
   * @brief Adds a chip directly into the library
   * @param chip entry to add to list
   */
  void AddChip(Chip chip);
  
  /**
   * @brief Checks if a chip with the same code exists in the library
   * @param chip to compare
   * @return true if both name and code match any chips in the library
   */
  bool IsChipValid(Chip& chip);

  /**
   * @brief Given a chip name, return all existing codes from the pool
   * @param chip to compare
   * @return list<char> codes
   */
  list<char> GetChipCodes(const Chip& chip);
  
  /**
   * @brief Given a name and code, return the chip from the database
   * @param name of the chip
   * @param code of the chip
   * @return Copy of entry. If no entry was found, a chip with error information is returned
   */
  Chip GetChipEntry(const std::string name, const char code);

protected:
 /**
  * @brief Reads in libary file and parses chip data
  * @important Will also parse chip scripts in the future before adding
  * @param path to library database
  *
  * If no chip data is found or if no data is successfully read, library will be empty
  *
  * Reads chip entry data and tries to add chips. It avoids duplicate entries. Chips
  * with the same name but different code do not count as duplicates. This allows the pool
  * to recognize and expect chips with particular chip codes.
  */
  void LoadLibrary(const std::string& path);

  /**
  * @brief Writes library to disc
  * @param path path to output file.
  * @warning will overwrite if path is already existing!
  * @return true if successful, false otherwise
  *
  * Writes the library as-is at the time of writing
  */
  const bool SaveLibrary(const std::string& path);

private:
  mutable list<Chip> library; /*!< the chip pool used by all chip resources */
};

#define CHIPLIB ChipLibrary::GetInstance()

