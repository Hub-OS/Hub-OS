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

std::unique_ptr<CardFolder> CardFolder::Clone() {
  auto clone = std::make_unique<CardFolder>();

  for (int i = 0; i < folderList.size(); i++) {
    clone->AddCard(*folderList[i]);
  }

  return std::move(clone);
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

Battle::Card* CardFolder::Skip()
{
    //Start with the card we want to replace. It's at the index we'd normally return with Next()
    Battle::Card* current = folderList[folderSize];
    //Grab the max we can't go past. For giga cards, it should be 27, but we might have less than that.
    int max = std::min(folderSize, 27);
    //Loop down over the folder list in the opposite direction of the hand.
    for (int count = max; count-- > 0; 0) {
        //Flip a weighted coin psuedorandomly.
        bool flip = (rand() % 10 + 1) > 7;
        //If the counted card is not a giga...
        if (folderList[count]->GetClass() != Battle::CardClass::giga) {
            //And the flip is true, OR we're at the end of our rope...
            if (flip || count == max) {
                //Swap the locations and break.
                folderList[folderSize] = folderList[count];
                folderList[count] = current;
                break;
            }
        }
    }
    return folderList[folderSize];
}

const int CardFolder::GetSize() const
{
  return folderSize;
}

CardFolder::Iter CardFolder::Begin()
{
  return folderList.begin();
}

CardFolder::Iter CardFolder::End()
{
  return folderList.end();
}
