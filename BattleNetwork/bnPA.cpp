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

const PASteps PA::GetMatchingSteps()
{
  PASteps result;

  for (int i = 0; i < iter->steps.size(); i++) {
    result.push_back(std::make_pair(iter->steps[i].cardShortName, iter->steps[i].code));
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
    
    PASteps steps;
    
    // Build the steps from each card in the entry
    for (auto&& step : entry.cards) {
      auto card = account.cards.find(step)->second;
      std::string name = account.cardProperties.find(card->modelId)->first;
      steps.push_back(std::make_pair(name, card->code));
    }

    if (steps.size() > 2) {
      // Valid combo recipe
      PAData data;
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
      continue; // try next 
    }


    int index = 0;
    while (index <= (int)size - (int)iter->steps.size()) {
      char code = input[index]->GetCode();

      //std::cout << "iter->steps[0].code == " << iter->steps[0].code << " | iter->steps[0].cardShortName == " << iter->steps[0].cardShortName << std::endl;
      //std::cout << "code == " << code << " | input[index]->GetShortName() == " << input[index]->GetShortName() << std::endl;

      if (iter->steps[0].code == code && iter->steps[0].cardShortName == input[index]->GetShortName()) {
        for (unsigned i = 0; i < iter->steps.size(); i++) {
          char code = input[i + index]->GetCode();

          if (iter->steps[i].code == code && iter->steps[i].cardShortName == input[i + index]->GetShortName()) {
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
      }

      index++;
    }

    if (match) {
      // Load the PA card
      if (advanceCardRef) { delete advanceCardRef; }

      advanceCardRef = new Battle::Card(WEBCLIENT.MakeBattleCardFromWebComboData(WebAccounts::CardCombo{ iter->uuid }));

      return startIndex;
    }

    // else keep looking
    startIndex = -1;
  }

  return startIndex;
}

