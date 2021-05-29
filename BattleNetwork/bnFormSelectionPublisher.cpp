#include "bnFormSelectionListener.hpp"
#include "bnFormSelectionPublisher.hpp"

FormSelectionPublisher::FormSelectionPublisher() = default;
FormSelectionPublisher::~FormSelectionPublisher() = default;

void FormSelectionPublisher::Broadcast(int index)
{
  for (auto&& listener : listeners)
  {
    listener->OnFormSelected(index);
  }
}
