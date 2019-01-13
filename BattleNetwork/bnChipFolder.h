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
  ChipFolder();
  ChipFolder(const ChipFolder& rhs);
  ~ChipFolder();
  void Shuffle();
  void AddChip(Chip copy);
  Chip* Next();
};

