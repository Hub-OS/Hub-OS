#include "bnFormSelectionListener.h"
#include "bnFormSelectionPublisher.h"

FormSelectionListener::FormSelectionListener() = default;
FormSelectionListener::~FormSelectionListener() = default;

void FormSelectionListener::Subscribe(FormSelectionPublisher& publisher)
{
  publisher.AddListener(this);
}
