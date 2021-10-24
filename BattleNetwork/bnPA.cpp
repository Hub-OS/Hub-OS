#include "bnPA.h"
#include "bnLogger.h"
#include "bnFileUtil.h"
#include <assert.h>
#include <iostream>
#include "bnCardLibrary.h"

PA::PA()
{
  advanceCardRef = nullptr;
}


PA::~PA()
{
  if (advanceCardRef) {
    // PA manages the memory it creates with `new`
    delete advanceCardRef;
    advanceCardRef = nullptr;
  }

  advances.clear();
}

void PA::RegisterPA(const PAData& entry)
{
  advances.push_back(entry);
}

const PA::Steps PA::GetMatchingSteps()
{
  PA::Steps result;

  for (int i = 0; i < iter->steps.size(); i++) {
    result.push_back(iter->steps[i]);
  }

  return result;
}

Battle::Card * PA::GetAdvanceCard()
{
  return advanceCardRef;
}

const int PA::FindPA(Battle::Card ** input, unsigned size)
{
  int startIndex = -1;

  if (size == 0) {
    return startIndex;
  }

  for (iter = advances.begin(); iter != advances.end(); iter++) {
    bool match = false;
    
    if (iter->steps.size() > size) {
      continue; // try next recipe
    }

    int index = 0;
    while (index <= (int)size - (int)iter->steps.size()) {
      for (unsigned i = 0; i < iter->steps.size(); i++) {
        char code_i = input[i + index]->GetCode();
        bool isSameCode = code_i == iter->steps[i].code;
        bool isWildStar = code_i == '*';
        bool isSameCard = iter->steps[i].name == input[i + index]->GetShortName();
        if ((isWildStar || isSameCode) && isSameCard) {
          match = true;
          // We do not break here. If it is a match all across the steps, then the for loop ends 
          // and match stays == true

          if (startIndex == -1) {
            startIndex = i + index;
          }
        }
        else {
          match = false;
          startIndex = -1;
          break;
        }
      }

      if (match) break;

      index++;
    }

    if (match) {
      // Load the PA card
      if (advanceCardRef) { delete advanceCardRef; }

      advanceCardRef = new Battle::Card({
        iter->uuid,
        iter->damage,
        0,
        '*',
        iter->canBoost,
        iter->timeFreeze,
        false,
        iter->name,
        iter->action,
        iter->action,
        "combo",
        iter->primaryElement,
        iter->secondElement,
        Battle::CardClass::standard,
        iter->metaClasses
      });

      return startIndex;
    }

    // else keep looking
    startIndex = -1;
  }

  return startIndex;
}

