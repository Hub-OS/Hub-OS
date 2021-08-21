#include "bnRealtimeCardUseListener.h"
#include "bnCardAction.h"
#include "bnCharacter.h"

void RealtimeCardActionUseListener::OnCardActionUsed(CardAction* action, uint64_t timestamp) {
  if (action->GetMetaData().timeFreeze == false) {
    action->GetActor().AddAction(CardEvent{ std::shared_ptr<CardAction>(action) }, ActionOrder::voluntary);
  }
}