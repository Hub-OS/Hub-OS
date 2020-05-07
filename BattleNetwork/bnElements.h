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
  ice,
  size
};

static const Element GetElementFromStr(std::string type)
{
    Element elemType;

    std::transform(type.begin(), type.end(), type.begin(), ::toupper);

    if (type == "FIRE") {
        elemType = Element::fire;
    }
    else if (type == "AQUA") {
        elemType = Element::aqua;
    }
    else if (type == "WOOD") {
        elemType = Element::wood;
    }
    else if (type == "ELEC" || type == "ELECTRIC") {
        elemType = Element::elec;
    }
    else if (type == "WIND") {
        elemType = Element::wind;
    }
    else if (type == "SWORD") {
        elemType = Element::sword;
    }
    else if (type == "BREAK") {
        elemType = Element::breaker;
    }
    else if (type == "CURSOR") {
        elemType = Element::cursor;
    }
    else if (type == "PLUS") {
        elemType = Element::plus;
    }
    else if (type == "SUMMON") {
        elemType = Element::summon;
    }
    else {
        elemType = Element::none;
    }

    return elemType;
}

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
    case Element::ice:
        res = "ICE";
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