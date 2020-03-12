#include "bnCardLibrary.h"
#include "bnFileUtil.h"
#include "bnTextureResourceManager.h"
#include <assert.h>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <iomanip>

CardLibrary::CardLibrary() {
  LoadLibrary("resources/database/library.txt");
}


CardLibrary::~CardLibrary() {
}

CardLibrary& CardLibrary::GetInstance() {
  static CardLibrary instance;
  return instance;
}

CardLibrary::Iter CardLibrary::Begin()
{
  return library.begin();
}

CardLibrary::Iter CardLibrary::End()
{
  return library.end();
}

const unsigned CardLibrary::GetSize() const
{
  return (unsigned)library.size();
}

void CardLibrary::AddCard(Card card)
{
  library.insert(card);
}

bool CardLibrary::IsCardValid(Card& card)
{
  for (auto i = Begin(); i != End(); i++) {
    if (i->GetShortName() == card.GetShortName() && i->GetCode() == card.GetCode()) {
      return true;
    }
  }

  return false;
}

std::list<char> CardLibrary::GetCardCodes(const Card& card)
{
  std::list<char> codes;

  for (auto i = Begin(); i != End(); i++) {
    if (i->GetShortName() == card.GetShortName()) {
      codes.insert(codes.begin(), i->GetCode());
    }
  }

  return codes;
}

const int CardLibrary::GetCountOf(const Card & card)
{
  return int(library.count(card));
}


void CardLibrary::LoadLibrary(const std::string& path) {
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

    if (line.find("Card") != string::npos) {
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
        longDescription = "This card does not have extra information.";
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

        // Card card = Card(atoi(cardID.c_str()), atoi(iconID.c_str()), code[0], atoi(damage.c_str()), elemType, name, description, longDescription, atoi(rarity.c_str()));
        // std::list<char> codes = this->GetCardCodes(card);

        /* Avoid code duplicates
        if (codes.size() > 0) {
          bool found = (std::find(codes.begin(), codes.end(), card.GetCode()) != codes.end());

          // Not a duplicate code, make sure information is correct
          if (!found) {
            // Simply update an existing card entry by changing the code 
            Card first = GetCardEntry(card.GetShortName(), *codes.begin());
            card = Card(first.GetID(), first.GetIconID(), card.GetCode(), first.GetDamage(), first.GetElement(), first.GetShortName(), first.GetDescription(), first.GetVerboseDescription(), first.GetRarity());
            library.insert(card);
          }
        }
        else { // first entry
           library.insert(card);
        }*/
        // library.insert(card);
      }
    }

    data = data.substr(endline + 1);
  } while (endline > -1);

  Logger::Log(std::string("library size: ") + std::to_string(this->GetSize()));
}

const bool CardLibrary::SaveLibrary(const std::string& path) {
  /**
   * Each card has a particular format
   * Lines beginning with pound '#' are comments and are ignored
   * Card name="ProtoMan" cardIndex="139" iconIndex="232" damage="120" type="Normal" codes="*,P" desc="Slices all enmy on field" "ProtoMan appears, stops time,\nand teleports to each open enemy striking once." rarity="5"
   */

  try {
    FileUtil::WriteStream ws(path);
    auto time = std::time(nullptr);
    auto timestamp = std::put_time(std::localtime(&time), "%y-%m-%d %OH:%OM:%OS");

    std::stringstream ss;
    ss << timestamp;
    std::string timestampStr = ss.str();

    ws << "# Saved on " << timestampStr << ws.endl();

    for (auto& card : library) {
      ws << "Card name=\"" << card.GetShortName() << "\" cardIndex=\""
         << card.GetUUID() << "\" ";
      ws << "\" damage=\"" << std::to_string(card.GetDamage()) << "\" ";
      ws << "type=\"" << GetStrFromElement(card.GetElement()) << "\" ";

      auto codes = CardLibrary::GetCardCodes(card);

      ws << "codes=\"";

      ws << card.GetCode();

      /*unsigned i = 0;
      for (auto code : codes) {
        ws << code;
        i++;

        if (i != codes.size() - 1) ws << ",";
      }*/

      ws << "\" ";

      ws << "desc=\"" << card.GetDescription() << "\" ";
      ws << "verbose=\""<< card.GetVerboseDescription() << "\" ";
      ws << "rarity=\"" << std::to_string(card.GetRarity()) << "\" ";

      ws << ws.endl();
    }

    Logger::Log(std::string("library saved successfully. Number of cards saved: ") +
                std::to_string(this->GetSize()));
    return true;
  } catch(std::exception& e) {
    Logger::Log(std::string("library save failed. Reason: ") + e.what());
  }

  return false;
}