#include "bnInvalidCardAction.h"
#include "bnParticlePoof.h"

InvalidCardAction::InvalidCardAction(std::shared_ptr<Character> actor) : CardAction(actor, "PLAYER_IDLE")
{
}

InvalidCardAction::~InvalidCardAction()
{
}

void InvalidCardAction::OnExecute(std::shared_ptr<Character> user)
{
  Battle::Tile* tile = user->GetTile();
  auto poof = std::make_shared<ParticlePoof>();
  poof->SetHeight(user->GetHeight());
  poof->SetLayer(-100); // in front of player and player widgets

  user->GetField()->AddEntity(poof, *tile);
}

void InvalidCardAction::OnAnimationEnd()
{
}

void InvalidCardAction::OnActionEnd()
{
}
