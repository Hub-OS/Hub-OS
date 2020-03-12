#pragma once

#include <string>
#include <cctype>
#include <algorithm>

/*! \brief Elements based on MMBN 6
 * 
 * http://megaman.wikia.com/wiki/Elements
 */

enum class Element : int {
  FIRE,
  AQUA,
  ELEC,
  WOOD,
  SWORD,
  WIND,
  CURSOR,
  SUMMON,  // Obstacles e.g. Blocks 
  PLUS,
  BREAK,
  NONE,
  ICE,
  SIZE
};

static const Element GetElementFromStr(std::string type)
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
    else if (type == "ELEC" || type == "ELECTRIC") {
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

static const std::string GetStrFromElement(const Element& type) {
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