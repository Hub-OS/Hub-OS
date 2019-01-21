#include "bnCounterHitListener.h"
#include "bnCounterHitPublisher.h"

void CounterHitListener::Subscribe(CounterHitPublisher& publisher) {
  publisher.AddListener(this);
}