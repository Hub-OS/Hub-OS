#include "bnCardUseListener.h"
#include "bnCardUsePublisher.h"

void CardUseListener::Subscribe(CardUsePublisher& publisher) {
  publisher.AddListener(this);
}