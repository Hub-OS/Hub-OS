#include "bnChip.h"
#include <iostream>
#include <algorithm>
#include <tuple>

Chip::Chip(unsigned id, unsigned icon, char code, unsigned damage, Element element, string sname, string desc, string verboseDesc, unsigned rarity) :
  ID(id), icon(icon), code(code), damage(damage), unmodDamage(damage), element(element) {
  this->shortname.assign(sname);
  this->description.assign(desc);
  this->verboseDescription.assign(verboseDesc);
  this->rarity = std::max((unsigned)1, rarity);
  this->rarity = std::min(this->rarity, (unsigned)5);
}

Chip::Chip() {

}

Chip::Chip(const Chip & copy) {
  ID = copy.ID;
  icon = copy.icon;
  code = copy.code;
  damage = copy.damage;
  unmodDamage = copy.unmodDamage;
  shortname = copy.shortname;
  description = copy.description;
  verboseDescription = copy.verboseDescription;
  element = copy.element;
  rarity = copy.rarity;
}

Chip::~Chip() {
  if (!description.empty()) {
    description.clear();
  }

  if (!shortname.empty()) {
    shortname.clear();
  }
}

const string Chip::GetVerboseDescription() const {
  return verboseDescription;
}

const string Chip::GetDescription() const {
  return description;
}

const string Chip::GetShortName() const {
  return shortname;
}

const char Chip::GetCode() const {
  return code;
}

const unsigned Chip::GetDamage() const {
  return damage;
}

const unsigned Chip::GetIconID() const
{
  return icon;
}

const unsigned Chip::GetID() const {
  return ID;
}

const Element Chip::GetElement() const
{
  return element;
}

const unsigned Chip::GetRarity() const
{
  return rarity;
}

bool Chip::Compare::operator()(const Chip & lhs, const Chip & rhs) const noexcept
{
  return lhs < rhs;;
}
