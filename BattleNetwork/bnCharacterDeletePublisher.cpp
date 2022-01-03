#include "bnCharacterDeletePublisher.h"
#include "bnCharacter.h"

CharacterDeletePublisher::~CharacterDeletePublisher()
{
}

void CharacterDeletePublisher::Broadcast(Character& pending)
{
  if (didOnce[pending.GetID()]) return;

  std::list<CharacterDeleteListener*>::iterator iter = listeners.begin();

  while (iter != listeners.end()) {
    (*iter)->OnDeleteEvent(pending);
    iter++;
  }

  didOnce[pending.GetID()] = true;
}
