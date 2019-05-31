#pragma once
#include "bnChip.h"
#include "bnChipLibrary.h"
#include <vector>
#include <algorithm>

/**
 * @class ChipFolder
 * @author mav
 * @date 05/05/19
 * @brief Reference of chip folder used by ChipFolderCollection
 * 
 * Provides utilities to list the chips in the folder, provide the next chip,
 * clone the folder, and shuffle the folder
 * 
 * TODO: any direct modification of this file is written to file by the collection class later
 */
class ChipFolder {
private:
  std::vector<Chip*> folderList; /*!< Chips */
  int folderSize; /*!< Size of the folder */
  int initialSize; /*!< Start of the folder size */

public:
  typedef std::vector<Chip*>::const_iterator Iter;

  /** 
   * @brief Empty folder
   */
  ChipFolder();
  
  /**
   * @brief Copy constructor
   * @warning chips in list are pointers and are allocated by the collection
   * 
   * DO NOT USE to clone folders to be exhausted in battle
   */
  ChipFolder(const ChipFolder& rhs);
  
  /**
   * @brief Deletes chip pointers at game close
   */
  ~ChipFolder();
  
  /**
   * @brief Randomly shuffles the folder
   */
  void Shuffle();
  
  /**
   * @brief Returns a safe clone of all chips used in the folder
   * @return ChipFolder*
   * @important Use Clone() to make a copy of the folder before exhausting it in battle
   */
  ChipFolder * Clone();
  
  /**
   * @brief Copies chip data and allocates new one in folder
   * @param copy
   */
  void AddChip(Chip copy);
  
  /**
   * @brief Moves the iterator over after returning chip pointer
   * @return Chip*
   */
  Chip* Next();
  
  /**
   * @brief The remaining size of the folder 
   * @return int
   */
  const int GetSize() const;
  
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
   * @brief Make a completely random and valid chip folder based on library entries
   * @return new ChipFolder
   * 
   * Used for testing
   */
  static ChipFolder MakeRandomFolder() {
    ChipFolder folder;
    folder.folderSize = folder.initialSize = ChipLibrary::GetInstance().GetSize();

    for (int i = 0; i < folder.folderSize; i++) {
      int random = rand() % ChipLibrary::GetInstance().GetSize();

      // the folder contains random parts from the entire library
      ChipLibrary::Iter iter = ChipLibrary::GetInstance().Begin();

      while (random-- > 0) {
        iter++;
      }

      folder.folderList.push_back(new Chip(*(iter)));
    }
  }
};

