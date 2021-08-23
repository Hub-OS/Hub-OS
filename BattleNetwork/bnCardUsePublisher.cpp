#include "bnCardUsePublisher.h"
#include "bnCardUseListener.h"

void CardActionUsePublisher::AddListener(CardActionUseListener* listener)
{
  if(auto iter = std::find(listeners.begin(), listeners.end(), listener); iter == listeners.end())
  listeners.push_back(listener);
}

void CardActionUsePublisher::DropSubscribers()
{
  listeners.clear();
}

CardActionUsePublisher::~CardActionUsePublisher()
{
}

void CardActionUsePublisher::Broadcast(std::shared_ptr<CardAction> action, uint64_t timestamp)
{
  std::vector<CardActionUseListener*>::iterator iter = listeners.begin();

  while (iter != listeners.end()) {
    (*iter)->OnCardActionUsed(action, timestamp);
    iter++;
  }
}
