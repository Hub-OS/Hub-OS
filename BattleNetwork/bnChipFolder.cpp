#include "bnChipFolder.h"
#include <assert.h>
#include <sstream>
#include <algorithm>

ChipFolder::ChipFolder() {
  folderSize = initialSize = 0;
}

ChipFolder::ChipFolder(const ChipFolder& rhs)
{
  folderSize = initialSize = rhs.folderSize;
  folderList = rhs.folderList;
}

ChipFolder::~ChipFolder() {
  for (int i = (int)folderList.size()-1; i >= 0; i--) {
    delete folderList[i];
  }
}

void ChipFolder::Shuffle()
{
  std::random_shuffle(folderList.begin(), folderList.end());
}

ChipFolder* ChipFolder::Clone() {
  ChipFolder* clone = new ChipFolder();

  for (int i = 0; i < folderList.size(); i++) {
    clone->AddChip(*folderList[i]);
  }

  return clone;
}

void ChipFolder::AddChip(Chip copy)
{
  folderList.push_back(new Chip(copy));
  folderSize = (int)folderList.size();
}

Chip* ChipFolder::Next() {
  if (--folderSize < 0) {
    return nullptr;
  }

  return folderList[folderSize];
}