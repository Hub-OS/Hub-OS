#include "bnChipFolder.h"
#include <assert.h>
#include <sstream>
#include <algorithm>

ChipFolder::ChipFolder() {
  folderSize = initialSize = 0;

  /*folderSize = initialSize = ChipLibrary::GetInstance().GetSize();

  for (int i = 0; i < folderSize; i++) {
    int random = rand() % ChipLibrary::GetInstance().GetSize();

    // For now, the folder contains random parts from the entire library
    ChipLibrary::Iter iter = ChipLibrary::GetInstance().Begin();

    while (random-- > 0) {
      iter++;
    }

    folderList.push_back(new Chip(*(iter)));
  }*/
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