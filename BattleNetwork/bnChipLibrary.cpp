#include "bnChipLibrary.h"
#include "bnFileUtil.h"
#include "bnTextureResourceManager.h"
#include <assert.h>
#include <sstream>
#include <algorithm>

ChipLibrary::ChipLibrary() {
  LoadLibrary("resources/database/library.txt");
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
  else if (type == "ELEC" || type == "ELECTRIC" ) {
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

const std::string ChipLibrary::GetStrFromElement(const Element type) {
  std::string res = "NONE";

  switch (type) {
  case Element::AQUA:
    res = "AQUA";
    break;
  case Element::BREAK:
    res = "BREAK";
    break;
  case Element::CURSOR:
    res = "CURSOR";
    break;
  case Element::ELEC:
    res = "ELEC";
    break;
  case Element::FIRE:
    res = "FIRE";
    break;
  case Element::ICE:
    res = "ICE";
    break;
  case Element::NONE:
    res = "NONE";
    break;
  case Element::PLUS:
    res = "PLUS";
    break;
  case Element::SUMMON:
    res = "SUMMON";
    break;
  case Element::SWORD:
    res = "SWORD";
    break;
  case Element::WIND:
    res = "WIND";
    break;
  case Element::WOOD:
    res = "WOOD";
    break;
  default:
    res = "NONE";
  }

  return res;
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

  return Chip(0, 0, code, 0, Element::NONE, name, "missing data", "This chip data could not be interpreted. It may come from another library and has not been configured properly to be used.", 1);
}

void ChipLibrary::LoadLibrary(const std::string& path) {
  string data = FileUtil::Read(path);

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

      string longDescription;

      try {
        longDescription = FileUtil::ValueOf("verbose", line);
      }
      catch (...) {
        longDescription = "This chip does not have extra information.";
      }

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

        Chip chip = Chip(atoi(cardID.c_str()), atoi(iconID.c_str()), code[0], atoi(damage.c_str()), elemType, name, description, longDescription, atoi(rarity.c_str()));
        std::list<char> codes = this->GetChipCodes(chip);

        // Avoid code duplicates
        if (codes.size() > 0) {
          bool found = (std::find(codes.begin(), codes.end(), chip.GetCode()) != codes.end());

          // Not a duplicate code, make sure information is correct
          if (!found) {
            // Simply update an existing chip entry by changing the code 
            Chip first = GetChipEntry(chip.GetShortName(), *codes.begin());
            chip = Chip(first.GetID(), first.GetIconID(), chip.GetCode(), first.GetDamage(), first.GetElement(), first.GetShortName(), first.GetDescription(), first.GetVerboseDescription(), first.GetRarity());
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

  std::reverse(library.begin(), library.end());

  Logger::Log(std::string("library size: ") + std::to_string(this->GetSize()));
}

const bool ChipLibrary::SaveLibrary(const std::string& path) {
  /**
   * Each chip has a particular format
   * Lines beginning with pound '#' are comments and are ignored
   * Chip name="ProtoMan" cardIndex="139" iconIndex="232" damage="120" type="Normal" codes="*,P" desc="Slices all enmy on field" "ProtoMan appears, stops time,\nand teleports to each open enemy striking once." rarity="5"
   */

  try {
    FileUtil::WriteStream ws(path);
    ws << "# Saved on " << "[timestamp here]" << ws.endl();

    for (auto chip : library) {
      ws << "Chip name=\"" << chip.GetShortName() << "\" cardIndex=\""
         << std::to_string(chip.GetID()) << "\" ";
      ws << "iconIndex=\"" << std::to_string(chip.GetIconID()) << "\" damage=\""
         << std::to_string(chip.GetDamage()) << "\" ";
      ws << "type=\"" << ChipLibrary::GetStrFromElement(chip.GetElement()) << "\" ";

      auto codes = ChipLibrary::GetChipCodes(chip);

      ws << "codes=\"";

      unsigned i = 0;
      for (auto code : codes) {
        ws << code << ",";
        i++;

        if (i != codes.size() - 1) ws << ",";
      }

      ws << "\" ";

      ws << "desc=\"" << chip.GetDescription() << "\" ";
      ws << "verbose=\""<< chip.GetVerboseDescription() << "\" ";
      ws << "rarity=\"" << std::to_string(chip.GetRarity()) << "\" ";

      ws << ws.endl();
    }

    Logger::Log(std::string("library saved successfully. Number of chips saved: ") +
                std::to_string(this->GetSize()));
    return true;
  } catch(std::exception& e) {
    Logger::Log(std::string("library save failed. Reason: ") + e.what());
  }

  return false;
}