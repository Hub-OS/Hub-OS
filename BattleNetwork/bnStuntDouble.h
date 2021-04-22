#pragma once
#include "bnCharacter.h"

class StuntDouble : public Character {
public:
  StuntDouble(Character& ref);
  ~StuntDouble();

  void OnDelete();
  void OnUpdate(double elapsed);
};