#include "bnCharacterSpawnPublisher.h"
#include "bnCharacter.h"

CharacterSpawnPublisher::~CharacterSpawnPublisher()
{
}

void CharacterSpawnPublisher::Broadcast(std::shared_ptr<Character>& pending)
{
  std::list<CharacterSpawnListener*>::iterator iter = listeners.begin();

  while (iter != listeners.end()) {
    (*iter)->OnSpawnEvent(pending);
    iter++;
  }
}
