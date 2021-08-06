#pragma once
#include "bnCard.h"
#include "bnCardLibrary.h"
#include <vector>
#include <algorithm>

/**
 * @class CardFolder
 * @author mav
 * @date 05/05/19
 * @brief Reference of card folder used by CardFolderCollection
 * 
 * Provides utilities to list the cards in the folder, provide the next card,
 * clone the folder, and shuffle the folder
 */
class CardFolder {
private:
  std::vector<Battle::Card*> folderList; /*!< Cards */
  int folderSize; /*!< Size of the folder */
  int initialSize; /*!< Start of the folder size */
  std::vector<std::string> errors; /*!< Error messages from folder source provider*/
public:
  typedef std::vector<Battle::Card*>::const_iterator Iter;

  /** 
   * @brief Empty folder
   */
  CardFolder();
  
  /**
   * @brief Copy constructor
   * @warning cards in list are pointers and are allocated by the collection
   * 
   * DO NOT USE to clone folders to be exhausted in battle
   */
  CardFolder(const CardFolder& rhs);
  
  /**
   * @brief Deletes card pointers at game close
   */
  ~CardFolder();
  
  /**
   * @brief Randomly shuffles the folder
   */
  void Shuffle();
  
  /**
   * @brief Returns a safe clone of all cards used in the folder
   * @return CardFolder*
   * @important Use Clone() to make a copy of the folder before exhausting it in battle
   */
  CardFolder * Clone();
  
  /**
   * @brief Copies card data and allocates new one in folder
   * @param copy
   */
  void AddCard(Battle::Card copy);
  
  /**
   * @brief Moves the iterator over after returning card pointer
   * @return Card*
   */
  Battle::Card* Next();
  
  /**
   * @brief The remaining size of the folder
   * @return int
   */
  const int GetSize() const;

  /**
   * @brief Reduce the remaining size of the folder.
   */
  void ReduceSize() { this->folderSize--; };
  
  /**
   * @brief Copies error messages into the folder
   * @param copy
   */
  void SetErrors(std::vector<std::string> copy);

  /**
   * @brief Query if this folder has an error
   * @return bool
   */
  const bool HasErrors() const;

  /**
   * @brief The remaining size of the folder
   * @return int
   */
  const std::vector<std::string>& GetErrors() const;

  /**
   * @brief Iterator to beginning of the folder
   * @return Iter
   */
  Iter Begin();
  
  /**
   * @brief Iterator to end of the folder
   * @return Iter
   */
  Iter End();

  /**
   * @brief Make a completely random and valid card folder based on library entries
   * @return new CardFolder
   * 
   * Used for testing
   */
  static CardFolder MakeRandomFolder() {
    CardFolder folder;
    folder.folderSize = folder.initialSize = CardLibrary::GetInstance().GetSize();

    for (int i = 0; i < folder.folderSize; i++) {
      int random = rand() % CardLibrary::GetInstance().GetSize();

      // the folder contains random parts from the entire library
      CardLibrary::Iter iter = CardLibrary::GetInstance().Begin();

      while (random-- > 0) {
        iter++;
      }

      folder.folderList.push_back(new Battle::Card(*(iter)));
    }

    return folder;
  }
};

