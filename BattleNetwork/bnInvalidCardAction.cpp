#include "bnInvalidCardAction.h"
#include "bnParticlePoof.h"

InvalidCardAction::InvalidCardAction(Character& actor) : CardAction(actor, "PLAYER_IDLE")
{
}

InvalidCardAction::~InvalidCardAction()
{
}

void InvalidCardAction::OnExecute(Character* user)
{
  Battle::Tile* tile = user->GetTile();
  ParticlePoof* poof = new ParticlePoof();
  poof->SetHeight(user->GetHeight());
  poof->SetLayer(-100); // in front of player and player widgets

  user->GetField()->AddEntity(*poof, *tile);
}

void InvalidCardAction::OnAnimationEnd()
{
}

void InvalidCardAction::OnActionEnd()
{
}
