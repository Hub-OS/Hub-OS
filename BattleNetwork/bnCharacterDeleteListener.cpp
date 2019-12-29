#include "bnCharacterDeleteListener.h"
#include "bnCharacterDeletePublisher.h"

void CharacterDeleteListener::Subscribe(CharacterDeletePublisher& publisher) {
  publisher.AddListener(this);
}