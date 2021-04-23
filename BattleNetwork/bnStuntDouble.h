#pragma once
#include "bnCharacter.h"

class StuntDouble : public Character {
  sf::Color defaultColor;
  Character& ref;
public:
  StuntDouble(Character& ref);
  ~StuntDouble();

  void OnDelete();
  void OnUpdate(double elapsed);
};