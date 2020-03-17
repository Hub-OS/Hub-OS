#include "bnCard.h"
#include <iostream>
#include <algorithm>
#include <tuple>

namespace Battle {
    Card::Card(std::string uuid, char code, unsigned damage, Element element, string sname, string desc, string verboseDesc, unsigned rarity) :
        uuid(uuid), code(code), damage(damage), unmodDamage(damage), element(element), secondaryElement(Element::NONE),
        navi(false), support(false), timeFreeze(false)
    {
        this->shortname.assign(sname);
        this->description.assign(desc);
        this->verboseDescription.assign(verboseDesc);
        this->rarity = std::max((unsigned)1, rarity);
        this->rarity = std::min(this->rarity, (unsigned)5);
    }

    Card::Card() {

    }

    Card::Card(const Battle::Card & copy) {
        uuid = copy.uuid;
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

    Card::~Card() {
        if (!description.empty()) {
            description.clear();
        }

        if (!shortname.empty()) {
            shortname.clear();
        }

        metaTags.clear();
    }

    const string Card::GetVerboseDescription() const {
        return verboseDescription;
    }

    const string Card::GetDescription() const {
        return description;
    }

    const string Card::GetShortName() const {
        return shortname;
    }

    const char Card::GetCode() const {
        return code;
    }

    const unsigned Card::GetDamage() const {
        return damage;
    }

    const std::string Card::GetUUID() const {
        return uuid;
    }

    const Element Card::GetElement() const
    {
        return element;
    }

    const Element Card::GetSecondaryElement() const
    {
        return (element == secondaryElement) ? Element::NONE : secondaryElement;
    }

    const unsigned Card::GetRarity() const
    {
        return rarity;
    }

    const bool Card::IsNaviSummon() const
    {
        return navi;
    }

    const bool Card::IsSupport() const
    {
        return support;
    }

    const bool Card::IsTimeFreeze() const
    {
        return timeFreeze;
    }

    const bool Card::IsTaggedAs(const std::string meta) const
    {
        return metaTags.find(meta) != metaTags.end();
    }

    bool Card::Compare::operator()(const Battle::Card & lhs, const Battle::Card & rhs) const noexcept
    {
        return lhs < rhs;;
    }
}