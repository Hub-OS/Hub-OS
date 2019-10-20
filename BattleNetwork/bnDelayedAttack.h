#pragma once
#include "bnSpell.h"

class DelayedAttack : public Spell {
private:
  double duration;
  Spell* next;

public:
  DelayedAttack(Spell* next, Battle::Tile::Highlight highlightMode, double seconds) : duration(seconds),
  Spell(next->GetField(), next->GetTeam()) {
    this->HighlightTile(highlightMode);
    this->SetFloatShoe(true);
    this->next = next;
  }

  ~DelayedAttack() { ;  }

  void OnUpdate(float elapsed) {
    duration -= elapsed;

    if (duration <= 0 && next) {
      GetField()->AddEntity(*next, *GetTile());
      this->Delete();
      next = nullptr;
    }
  }

  void Attack(Character* _entity) { }
};