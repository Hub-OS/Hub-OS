#include "bnInvisCardAction.h"
#include "bnInvis.h"
#include "bnCharacter.h"

InvisCardAction::InvisCardAction(Character* actor) : CardAction(actor, "PLAYER_IDLE")
{
}

InvisCardAction::~InvisCardAction()
{
}

void InvisCardAction::OnExecute(Character* user)
{
  user->CreateComponent<Invis>(user)->Update(0);
}

void InvisCardAction::OnActionEnd()
{
}

void InvisCardAction::OnAnimationEnd()
{
}
