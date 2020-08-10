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

PA PA::ReadFromWebAccount(const WebAccounts::AccountState& account)
{
  PA pa;

  // Loop through each combo entry from the API
  for (auto&& pair : account.cardCombos) {
    auto entry = *pair.second;
    
    PA::Steps steps;
    
    // Build the steps from each card in the entry
    for (auto&& step : entry.cards) {
      auto cardIter = account.cards.find(step);
      
      if (cardIter == account.cards.end()) continue;
      
      auto modelIter = account.cardProperties.find(cardIter->second->modelId);

      if (modelIter == account.cardProperties.end()) continue;

      auto card = cardIter->second;
      auto model = modelIter->second;

      steps.push_back({ card->id, model->name, card->code });
    }

    if (steps.size() > 2) {
      // Valid combo recipe
      PAData data;
      
      data.steps = steps;
      data.action = entry.action;
      data.canBoost = entry.canBoost;
      data.damage = entry.damage;
      data.metaClasses = entry.metaClasses;
      data.name = entry.name;
      data.primaryElement = GetElementFromStr(entry.element);
      data.secondElement = GetElementFromStr(entry.secondaryElement);
      data.timeFreeze = entry.timeFreeze;
      data.uuid = entry.id;

      // Add to our PA object
      pa.RegisterPA(data);
    }
  }

  return pa;
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

      index++;
    }

    if (match) {
      // Load the PA card
      if (advanceCardRef) { delete advanceCardRef; }

      auto fromUUID = WebAccounts::CardCombo{ iter->uuid };
      advanceCardRef = new Battle::Card(WEBCLIENT.MakeBattleCardFromWebComboData(fromUUID));

      return startIndex;
    }

    // else keep looking
    startIndex = -1;
  }

  return startIndex;
}

