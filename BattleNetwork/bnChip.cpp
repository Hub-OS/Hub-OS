#include "bnChip.h"
#include <iostream>
#include <algorithm>
#include <tuple>

Chip::Chip(unsigned id, unsigned icon, char code, unsigned damage, Element element, string sname, string desc, string verboseDesc, unsigned rarity) :
  ID(id), icon(icon), code(code), damage(damage), unmodDamage(damage), element(element), secondaryElement(Element::NONE),
  navi(false), support(false), timeFreeze(false)
{
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
  secondaryElement = copy.secondaryElement;
  navi = copy.navi;
  support = copy.support;
  timeFreeze = copy.timeFreeze;
  rarity = copy.rarity;
  metaTags = copy.metaTags;
}

Chip::~Chip() {
  if (!description.empty()) {
    description.clear();
  }

  if (!shortname.empty()) {
    shortname.clear();
  }

  metaTags.clear();
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

const Element Chip::GetSecondaryElement() const
{
  return (element == secondaryElement) ? Element::NONE : secondaryElement;
}

const unsigned Chip::GetRarity() const
{
  return rarity;
}

const bool Chip::IsNaviSummon() const
{
  return navi;
}

const bool Chip::IsSupport() const
{
  return support;
}

const bool Chip::IsTimeFreeze() const
{
  return timeFreeze;
}

const bool Chip::IsTaggedAs(const std::string meta) const
{
  return metaTags.find(meta) != metaTags.end();
}

bool Chip::Compare::operator()(const Chip & lhs, const Chip & rhs) const noexcept
{
  return lhs < rhs;;
}
