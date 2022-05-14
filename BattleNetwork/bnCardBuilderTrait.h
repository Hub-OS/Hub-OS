#pragma once
#include <memory>
#include "bnCardAction.h"

class CardBuilderTrait {
public:
  virtual ~CardBuilderTrait() {};
  virtual std::shared_ptr<CardAction> BuildCardAction(std::shared_ptr<Character> user, const Battle::Card::Properties& props) = 0;
  virtual void OnUpdate(Battle::Card::Properties& props, double elapsed) = 0;
};