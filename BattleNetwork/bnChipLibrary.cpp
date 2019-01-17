#include "bnChipLibrary.h"
#include "bnFileUtil.h"
#include "bnTextureResourceManager.h"
#include <assert.h>
#include <sstream>
#include <algorithm>

ChipLibrary::ChipLibrary() {
  LoadLibrary();
}


ChipLibrary::~ChipLibrary() {
}

ChipLibrary& ChipLibrary::GetInstance() {
  static ChipLibrary instance;
  return instance;
}

ChipLibrary::Iter ChipLibrary::Begin()
{
  return library.begin();
}

ChipLibrary::Iter ChipLibrary::End()
{
  return library.end();
}

const unsigned ChipLibrary::GetSize() const
{
  return (unsigned)library.size();
}

const Element ChipLibrary::GetElementFromStr(std::string type)
{
  Element elemType;

  std::transform(type.begin(), type.end(), type.begin(), ::toupper);

  if (type == "FIRE") {
    elemType = Element::FIRE;
  }
  else if (type == "AQUA") {
    elemType = Element::AQUA;
  }
  else if (type == "WOOD") {
    elemType = Element::WOOD;
  }
  else if (type == "ELEC") {
    elemType = Element::ELEC;
  }
  else if (type == "WIND") {
    elemType = Element::WIND;
  }
  else if (type == "SWORD") {
    elemType = Element::SWORD;
  }
  else if (type == "BREAK") {
    elemType = Element::BREAK;
  }
  else if (type == "CURSOR") {
    elemType = Element::CURSOR;
  }
  else if (type == "PLUS") {
    elemType = Element::PLUS;
  }
  else if (type == "SUMMON") {
    elemType = Element::SUMMON;
  }
  else {
    elemType = Element::NONE;
  }

  return elemType;
}

void ChipLibrary::AddChip(Chip chip)
{
  library.push_back(chip);
}

bool ChipLibrary::IsChipValid(Chip& chip)
{
  for (auto i = Begin(); i != End(); i++) {
    if (i->GetShortName() == chip.GetShortName() && i->GetCode() == chip.GetCode()) {
      return true;
    }
  }

  return false;
}

std::list<char> ChipLibrary::GetChipCodes(const Chip& chip)
{
  std::list<char> codes;

  for (auto i = Begin(); i != End(); i++) {
    if (i->GetShortName() == chip.GetShortName()) {
      codes.insert(codes.begin(), i->GetCode());
    }
  }

  return codes;
}

Chip ChipLibrary::GetChipEntry(const std::string name, const char code)
{
  for (auto i = Begin(); i != End(); i++) {
    if (i->GetShortName() == name && i->GetCode() == code) {
      return (*i);
    }
  }

  return Chip(0, 0, code, 0, Element::NONE, name, "missing data", 1);
}

// Used as the folder in battle
void ChipLibrary::LoadLibrary() {
  // TODO: put this utility in an input stream class and inhert from that
  string data = FileUtil::Read("resources/database/library.txt");

  int endline = 0;

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

    if (line.find("Chip") != string::npos) {
      string cardID = FileUtil::ValueOf("cardIndex", line);
      string iconID = FileUtil::ValueOf("iconIndex", line);
      string name   = FileUtil::ValueOf("name", line);
      string damage = FileUtil::ValueOf("damage", line);
      string type = FileUtil::ValueOf("type", line);
      string codes = FileUtil::ValueOf("codes", line);
      string description = FileUtil::ValueOf("desc", line);
      string rarity = FileUtil::ValueOf("rarity", line);

      // Trime white space
      codes.erase(remove_if(codes.begin(), codes.end(), isspace), codes.end());

      // Tokenize the string with delimeter as ','
      std::istringstream codeStream(codes);
      string code;

      while (std::getline(codeStream, code, ',')) {
        // For every code, push this into our database
        if (code.empty())
          continue;

        Element elemType = GetElementFromStr(type);

        Chip chip = Chip(atoi(cardID.c_str()), atoi(iconID.c_str()), code[0], atoi(damage.c_str()), elemType, name, description, atoi(rarity.c_str()));
        std::list<char> codes = this->GetChipCodes(chip);

        // Avoid code duplicates
        if (codes.size() > 0) {
          bool found = (std::find(codes.begin(), codes.end(), chip.GetCode()) != codes.end());

          // Not a duplicate code, make sure information is correct
          if (!found) {
            // Simply update an existing chip entry by changing the code 
            Chip first = GetChipEntry(chip.GetShortName(), *codes.begin());
            chip = Chip(first.GetID(), first.GetIconID(), chip.GetCode(), first.GetDamage(), first.GetElement(), first.GetShortName(), first.GetDescription(), first.GetRarity());
            library.push_back(chip);
          }
        }
        else { // first entry
          library.push_back(chip);
        }
      }
    }

    data = data.substr(endline + 1);
  } while (endline > -1);

  std::cout << "library size: " << this->GetSize() << std::endl;;
}