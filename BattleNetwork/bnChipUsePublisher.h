#include <list>

#include "bnComponent.h"
#include "bnChip.h"
#include "bnChipUseListener.h"

class ChipUsePublisher {
private:
  friend class ChipUseListener;

  std::list<ChipUseListener*> listeners;

  void AddListener(ChipUseListener* listener) {
    listeners.push_back(listener);
  }

public:
  virtual void UseNextChip() = 0;

  void Broadcast(Chip& chip) {
    std::list<ChipUseListener*>::iterator iter = listeners.begin();

    while (iter != listeners.end()) {
      (*iter)->OnChipUse(chip);
      iter++;
    }
  }
};