#include "bnCardUseListener.h"
#include "bnCardUsePublisher.h"

void CardActionUseListener::Subscribe(CardActionUsePublisher& publisher) {
  publisher.AddListener(this);
}