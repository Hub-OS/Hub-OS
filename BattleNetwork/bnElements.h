#pragma once

#include <string>
#include <cctype>
#include <algorithm>

/*! \brief Elements based on MMBN 6
 * 
 * http://megaman.wikia.com/wiki/Elements
 */

enum class Element : int {
  fire,
  aqua,
  elec,
  wood,
  sword,
  wind,
  cursor,
  summon,  // Obstacles e.g. Blocks 
  plus,
  breaker,
  none,
  size
};

static const Element GetElementFromStr(const std::string& type)
{
  Element elemType;

  std::string temp = type;
  std::transform(temp.begin(), temp.end(), temp.begin(), ::toupper);

  if (temp == "FIRE") {
    elemType = Element::fire;
  }
  else if (temp == "AQUA") {
    elemType = Element::aqua;
  }
  else if (temp == "WOOD") {
    elemType = Element::wood;
  }
  else if (temp == "ELEC" || temp == "ELECTRIC") {
    elemType = Element::elec;
  }
  else if (temp == "WIND") {
    elemType = Element::wind;
  }
  else if (temp == "SWORD") {
    elemType = Element::sword;
  }
  else if (temp == "BREAK") {
    elemType = Element::breaker;
  }
  else if (temp == "CURSOR") {
    elemType = Element::cursor;
  }
  else if (temp == "PLUS") {
    elemType = Element::plus;
  }
  else if (temp == "SUMMON") {
    elemType = Element::summon;
  }
  else {
    elemType = Element::none;
  }

  return elemType;
}

/*static const Element GetElementFromStr(const char* type) {
  return GetElementFromStr(std::string(type));
}*/

static const std::string GetStrFromElement(const Element& type) {
  std::string res = "NONE";

  switch (type) {
  case Element::aqua:
    res = "AQUA";
    break;
  case Element::breaker:
    res = "BREAK";
    break;
  case Element::cursor:
    res = "CURSOR";
    break;
  case Element::elec:
    res = "ELEC";
    break;
  case Element::fire:
    res = "FIRE";
    break;
  case Element::none:
    res = "NONE";
    break;
  case Element::plus:
    res = "PLUS";
    break;
  case Element::summon:
    res = "SUMMON";
    break;
  case Element::sword:
    res = "SWORD";
    break;
  case Element::wind:
    res = "WIND";
    break;
  case Element::wood:
    res = "WOOD";
    break;
  default:
    res = "NONE";
  }

  return res;
}