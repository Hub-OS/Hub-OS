#include "bnFormSelectionListener.hpp"
#include "bnFormSelectionPublisher.hpp"

FormSelectionListener::FormSelectionListener() = default;
FormSelectionListener::~FormSelectionListener() = default;

void FormSelectionListener::Subscribe(FormSelectionPublisher& publisher)
{
  publisher.AddListener(this);
}
