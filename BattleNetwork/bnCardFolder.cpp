#include "bnCardFolder.h"
#include <assert.h>
#include <sstream>
#include <algorithm>
#include <random>

CardFolder::CardFolder() {
  folderSize = initialSize = 0;
}

CardFolder::CardFolder(const CardFolder& rhs)
{
  folderSize = initialSize = rhs.folderSize;
  folderList = rhs.folderList;
}

CardFolder::~CardFolder() {
  for (int i = (int)folderList.size()-1; i >= 0; i--) {
    delete folderList[i];
  }
}

void CardFolder::Shuffle()
{
  std::random_device rng;
  std::mt19937 urng(rng());
  std::shuffle(folderList.begin(), folderList.end(), urng);
}

CardFolder* CardFolder::Clone() {
  CardFolder* clone = new CardFolder();

  for (int i = 0; i < folderList.size(); i++) {
    clone->AddCard(*folderList[i]);
  }

  return clone;
}

void CardFolder::AddCard(Battle::Card copy)
{
  folderList.push_back(new Battle::Card(copy));
  folderSize = (int)folderList.size();
}

Battle::Card* CardFolder::Next() {
  if (--folderSize < 0) {
    return nullptr;
  }

  return folderList[folderSize];
}

const int CardFolder::GetSize() const
{
  return this->folderSize;
}

CardFolder::Iter CardFolder::Begin()
{
  return folderList.begin();
}

CardFolder::Iter CardFolder::End()
{
  return folderList.end();
}
