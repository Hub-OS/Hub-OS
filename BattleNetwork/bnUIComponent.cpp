#include "bnUIComponent.h"

void UIComponent::SetDrawOnUIPass(bool enabled)
{
  autodraw = enabled;
}

const bool UIComponent::DrawOnUIPass() const
{
    return autodraw;
}
