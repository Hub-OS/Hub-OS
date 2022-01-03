#include "bnDelayedAttack.h"
#include "bnField.h"

DelayedAttack::DelayedAttack(std::shared_ptr<Entity> next, Battle::TileHighlight highlightMode, double seconds) : 
  duration(seconds),
  Spell(next->GetTeam())
{
  HighlightTile(highlightMode);
  SetFloatShoe(true);
  DelayedAttack::next = next;
}

DelayedAttack::~DelayedAttack()
{
}

void DelayedAttack::OnUpdate(double elapsed)
{
    duration -= elapsed;

    if (duration <= 0 && next) {
        GetField()->AddEntity(next, *GetTile());
        Delete();
        next = nullptr;
    }
}

void DelayedAttack::Attack(std::shared_ptr<Entity> _entity)
{
}

void DelayedAttack::OnDelete()
{
  Erase();
}
