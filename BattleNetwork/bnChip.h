#pragma once

#include <string>
#include "bnElements.h"

using std::string;

// TODO: Write Chip component class to attach to Player
// Loaded chips from chip select GUI pipes into this component
class Chip {
public:
  Chip(unsigned id, unsigned icon, char code, unsigned damage, Element element, string sname, string desc, string verboseDesc, unsigned rarity);
  Chip(const Chip& copy);
  Chip();
  ~Chip();
  const string GetVerboseDescription() const;
  const string GetDescription() const;
  const string GetShortName() const;
  const char GetCode() const;
  const unsigned GetDamage() const;
  const unsigned GetIconID() const;
  const unsigned GetID() const;
  const Element GetElement() const;
  const unsigned GetRarity() const;
private:
  unsigned ID;
  unsigned icon;
  unsigned damage;
  unsigned rarity;
  char code;
  string shortname;
  string description;
  string verboseDescription;
  Element element;
};

