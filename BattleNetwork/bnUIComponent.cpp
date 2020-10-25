#include "bnUIComponent.h"

void UIComponent::SetAutoDraw(bool enabled)
{
  autodraw = enabled;
}

const bool UIComponent::AutoDraw() const
{
    return autodraw;
}
