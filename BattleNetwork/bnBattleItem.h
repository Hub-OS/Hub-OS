#pragma once

#include <string>
#include "bnChip.h"

class BattleItem {
private:
  int cardID;
  std::string name;
  bool isChip;
  bool isZenny;
  bool isHP;
  Chip chip;
public:
  BattleItem(std::string name, int id) : name(name), cardID(id), chip(0, 0, 0, 0, Element::NONE, "null", "null", 0) { isChip = isZenny = isHP = false; }
  BattleItem(Chip chip) : chip(chip), name(chip.GetShortName()), cardID(chip.GetID()) { isChip = true; isZenny = isHP = false; }
  BattleItem(const BattleItem& rhs) : chip(rhs.chip) { isChip = rhs.isChip; isZenny = rhs.isZenny; isHP = rhs.isHP; cardID = rhs.cardID; name = rhs.name; }
  int GetID() { return cardID; }
  const std::string GetName() { return name;  }
  bool IsChip() { return isChip; }
  Chip GetChip() { return chip; }
  char GetChipCode() {
    return chip.GetCode();
  }
};