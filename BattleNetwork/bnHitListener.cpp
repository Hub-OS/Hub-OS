#include "bnHitListener.h"
#include "bnHitPublisher.h"

void HitListener::Subscribe(HitPublisher& publisher) {
  publisher.AddListener(this);
}