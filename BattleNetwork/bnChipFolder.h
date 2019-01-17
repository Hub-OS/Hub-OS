#pragma once
#include "bnChip.h"
#include "bnChipLibrary.h"
#include <vector>
#include <algorithm>

class ChipFolder {
private:
  std::vector<Chip*> folderList;
  int folderSize;
  int initialSize;

public:
  typedef std::vector<Chip*>::const_iterator Iter;

  ChipFolder();
  ChipFolder(const ChipFolder& rhs);
  ~ChipFolder();
  void Shuffle();
  ChipFolder * Clone();
  void AddChip(Chip copy);
  Chip* Next();
  const int GetSize() const;
  Iter Begin();
  Iter End();

  // Make a completely random and valid chip folder (tests)
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

