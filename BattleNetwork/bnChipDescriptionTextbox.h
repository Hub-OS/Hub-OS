#pragma once

#include "bnChipUseListener.h"
#include "bnAnimatedTextBox.h"

class ChipDescriptionTextbox : public AnimatedTextBox {
public:
  ChipDescriptionTextbox(sf::Vector2f pos);
  void DescribeChip(Chip* chip);
};