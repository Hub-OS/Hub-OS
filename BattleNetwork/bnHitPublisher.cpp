#include "bnHitPublisher.h"
#include "bnHitListener.h"

HitPublisher::~HitPublisher()
{
}

void HitPublisher::Broadcast(Character& victim, const Hit::Properties& props)
{
  std::list<HitListener*>::iterator iter = listeners.begin();

  while (iter != listeners.end()) {
    (*iter)->OnHit(victim, props);
    iter++;
  }
}
