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

void PA::LoadPA()
{
  advances.clear();

  string data = FileUtil::Read("resources/database/PA.txt");

  int endline = 0;
  std::vector<PA::PAData::Required> currSteps;
  std::string currPA;
  std::string damage;
  std::string icon;
  std::string type;

  do {
    endline = (int)data.find("\n");
    string line = data.substr(0, endline);

    while (line.compare(0, 1, " ") == 0)
      line.erase(line.begin()); // remove leading whitespaces
    while (line.size() > 0 && line.compare(line.size() - 1, 1, " ") == 0)
      line.erase(line.end() - 1); // remove trailing whitespaces

    if (line[0] == '#') {
      // Skip comments
      data = data.substr(endline + 1);
      continue;
    }

    if (line.find("PA") != string::npos) {
      if (!currSteps.empty()) {
        if (currSteps.size() > 1) {
          Element elemType = GetElementFromStr(type);

          advances.push_back(PA::PAData({ currPA, (unsigned)atoi(icon.c_str()), (unsigned)atoi(damage.c_str()), elemType, currSteps }));
          currSteps.clear();
        }
        else {
          Logger::Log("Error. PA \"" + currPA + "\": only has 1 required card for recipe. PA's must have 2 or more cards. Skipping entry.");
          currSteps.clear();
        }
      }
      
      currPA = valueOf("name", line);
      damage = valueOf("damage", line);
      icon = valueOf("iconIndex", line);
      type = valueOf("type", line);
    } else if (line.find("Card") != string::npos) {
      string name = valueOf("name", line);
      string code = valueOf("code", line);

      currSteps.push_back(PA::PAData::Required({ name,code[0] }));

      //std::cout << "card step: " << name << " " << code[0] << endl;
    }

    data = data.substr(endline + 1);
  } while (endline > -1);

  if (currSteps.size() > 1) {
    Element elemType = GetElementFromStr(type);

    //std::cout << "PA entry 2: " << currPA << " " << (unsigned)atoi(icon.c_str()) << " " << (unsigned)atoi(damage.c_str()) << " " << type << endl;
    advances.push_back(PA::PAData({ currPA, (unsigned)atoi(icon.c_str()), (unsigned)atoi(damage.c_str()), elemType, currSteps }));
    currSteps.clear();
  }
  else {
    //std::cout << "Error. PA " + currPA + " only has 1 required card for recipe. PA's must have 2 or more cards. Skipping entry.\n";
    Logger::Log("Error. PA \"" + currPA + "\": only has 1 required card for recipe. PA's must have 2 or more cards. Skipping entry.");
    currSteps.clear();
  }
}

std::string PA::valueOf(std::string _key, std::string _line){
  int keyIndex = (int)_line.find(_key);
  assert(keyIndex > -1 && "Key was not found in PA file.");
  string s = _line.substr(keyIndex + _key.size() + 2);
  return s.substr(0, s.find("\""));
}

const PASteps PA::GetMatchingSteps()
{
  PASteps result;

  for (int i = 0; i < iter->steps.size(); i++) {
    result.push_back(std::make_pair(iter->steps[i].cardShortName, iter->steps[i].code));
  }

  return result;
}

Card * PA::GetAdvanceCard()
{
   return advanceCardRef;
}

const int PA::FindPA(Card ** input, unsigned size)
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

       advanceCardRef = new Card("PA", 0, iter->damage, iter->type, iter->name, "Program Advance", "", 0);

      return startIndex;
    }

    // else keep looking
    startIndex = -1;
  }

  return startIndex;
}

