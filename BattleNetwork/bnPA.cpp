#include "bnPA.h"
#include "bnLogger.h"
#include "bnFileUtil.h"
#include <assert.h>
#include <iostream>
#include "bnChipLibrary.h"

PA::PA()
{
  advanceChipRef = nullptr;
}


PA::~PA()
{
  if (advanceChipRef) {
    // PA manages the memory it creates with `new`
    delete advanceChipRef;
    advanceChipRef = nullptr;
  }

  advances.clear();
}

void PA::LoadPA()
{
  advances.clear();

  // TODO: put this utility in an input stream class and inhert from that
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
          Element elemType = ChipLibrary::GetElementFromStr(type);

          advances.push_back(PA::PAData({ currPA, (unsigned)atoi(icon.c_str()), (unsigned)atoi(damage.c_str()), elemType, currSteps }));
          currSteps.clear();
        }
        else {
          Logger::Log("Error. PA \"" + currPA + "\": only has 1 required chip for recipe. PA's must have 2 or more chips. Skipping entry.");
          currSteps.clear();
        }
      }
      
      currPA = valueOf("name", line);
      damage = valueOf("damage", line);
      icon = valueOf("iconIndex", line);
      type = valueOf("type", line);
    } else if (line.find("Chip") != string::npos) {
      string name = valueOf("name", line);
      string code = valueOf("code", line);

      currSteps.push_back(PA::PAData::Required({ name,code[0] }));

      //std::cout << "chip step: " << name << " " << code[0] << endl;
    }

    data = data.substr(endline + 1);
  } while (endline > -1);

  if (currSteps.size() > 1) {
    Element elemType = ChipLibrary::GetElementFromStr(type);

    //std::cout << "PA entry 2: " << currPA << " " << (unsigned)atoi(icon.c_str()) << " " << (unsigned)atoi(damage.c_str()) << " " << type << endl;
    advances.push_back(PA::PAData({ currPA, (unsigned)atoi(icon.c_str()), (unsigned)atoi(damage.c_str()), elemType, currSteps }));
    currSteps.clear();
  }
  else {
    //std::cout << "Error. PA " + currPA + " only has 1 required chip for recipe. PA's must have 2 or more chips. Skipping entry.\n";
    Logger::Log("Error. PA \"" + currPA + "\": only has 1 required chip for recipe. PA's must have 2 or more chips. Skipping entry.");
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
    result.push_back(std::make_pair(iter->steps[i].chipShortName, iter->steps[i].code));
  }

  return result;
}

Chip * PA::GetAdvanceChip()
{
   return advanceChipRef;
}

const int PA::FindPA(Chip ** input, unsigned size)
{
  int startIndex = -1;

  std::cout << "size: " << size << "\n";

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

      //std::cout << "iter->steps[0].code == " << iter->steps[0].code << " | iter->steps[0].chipShortName == " << iter->steps[0].chipShortName << std::endl;
      //std::cout << "code == " << code << " | input[index]->GetShortName() == " << input[index]->GetShortName() << std::endl;

      if (iter->steps[0].code == code && iter->steps[0].chipShortName == input[index]->GetShortName()) {
        for (unsigned i = 0; i < iter->steps.size(); i++) {
          char code = input[i + index]->GetCode();

          if (iter->steps[i].code == code && iter->steps[i].chipShortName == input[i + index]->GetShortName()) {
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
      // Load the PA chip
      if (advanceChipRef) { delete advanceChipRef; }

       advanceChipRef = new Chip(0, iter->icon, 0, iter->damage, iter->type, iter->name, "Program Advance", 0);

      return startIndex;
    }

    // else keep looking
    startIndex = -1;
  }

  return startIndex;
}

