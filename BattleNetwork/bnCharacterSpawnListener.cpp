#include "bnCharacterSpawnListener.h"
#include "bnCharacterSpawnPublisher.h"

void CharacterSpawnListener::Subscribe(CharacterSpawnPublisher& publisher) {
  publisher.AddListener(this);
}