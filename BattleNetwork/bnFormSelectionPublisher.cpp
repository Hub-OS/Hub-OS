#include "bnFormSelectionListener.h"
#include "bnFormSelectionPublisher.h"

FormSelectionPublisher::FormSelectionPublisher() = default;
FormSelectionPublisher::~FormSelectionPublisher() = default;

void FormSelectionPublisher::Broadcast(int index)
{
  for (auto&& listener : listeners)
  {
    listener->OnFormSelected(index);
  }
}
