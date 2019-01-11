#pragma once

#include "bnChipUseListener.h"
#include "bnAnimatedTextBox.h"

class ChipDescriptionTextbox : public AnimatedTextBox {
public:
  ChipDescriptionTextbox(sf::Vector2f pos, sf::IntRect textArea);
  void DescribeChip(Chip* chip);
};