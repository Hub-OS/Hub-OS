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
      string cardID = valueOf("cardIndex", line);
      string iconID = valueOf("iconIndex", line);
      string name   = valueOf("name", line);
      string damage = valueOf("damage", line);
      string type = valueOf("type", line);
      string codes = valueOf("codes", line);
      string description = valueOf("desc", line);
      string rarity = valueOf("rarity", line);

      // Trime white space
      codes.erase(remove_if(codes.begin(), codes.end(), isspace), codes.end());

      // Tokenize the string with delimeter as ','
      std::istringstream codeStream(codes);
      string code;

      while (std::getline(codeStream, code, ',')) {
        // For every code, push this into our database
        if (code.empty())
          continue;

        if (code == "*") {
          // transform it into the font-compatible ascii char
          code = '=';
        }

        Element elemType = GetElementFromStr(type);

        library.push_back(Chip(atoi(cardID.c_str()), atoi(iconID.c_str()), code[0], atoi(damage.c_str()), elemType, name, description, atoi(rarity.c_str())));
      }
    }

    data = data.substr(endline + 1);
  } while (endline > -1);
}

string ChipLibrary::valueOf(string _key, string _line) {
  int keyIndex = (int)_line.find(_key);
  assert(keyIndex > -1 && "Key was not found in library file.");
  string s = _line.substr(keyIndex + _key.size() + 2);
  return s.substr(0, s.find("\""));
}