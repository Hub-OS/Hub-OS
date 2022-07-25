#pragma once
#include "bnCharacter.h"

class StuntDouble : public Character {
  sf::Color defaultColor;
  std::shared_ptr<Character> ref;
public:
  StuntDouble(std::shared_ptr<Character> ref);
  ~StuntDouble();

  void Init() override;
  void OnDelete() override;
  void OnUpdate(double elapsed);
  bool CanMoveTo(Battle::Tile*) override;
};