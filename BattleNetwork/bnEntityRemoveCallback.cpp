#include "bnEntityRemoveCallback.h"
#include "bnEntity.h"
EntityRemoveCallback::EntityRemoveCallback(Entity& owner) : 
  Callback<void(Entity*)>(),
  owner(owner)
{}

EntityRemoveCallback::~EntityRemoveCallback()
{
  if (this->CanUse()) {
    owner.ForgetRemoveCallback(*this);
  }
}
