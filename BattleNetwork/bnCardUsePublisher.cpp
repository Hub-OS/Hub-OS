#include "bnCardUsePublisher.h"
#include "bnCardUseListener.h"

void CardUsePublisher::AddListener(CardUseListener* listener)
{
  listeners.push_back(listener);
}

void CardUsePublisher::DropSubscribers()
{
  listeners.clear();
}

CardUsePublisher::~CardUsePublisher()
{
}

void CardUsePublisher::Broadcast(const Battle::Card& card, Character& user, uint64_t timestamp)
{
  std::list<CardUseListener*>::iterator iter = listeners.begin();

  while (iter != listeners.end()) {
    (*iter)->OnCardUse(card, user, timestamp);
    iter++;
  }
}
