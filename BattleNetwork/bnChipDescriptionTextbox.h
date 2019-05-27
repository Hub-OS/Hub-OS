#pragma once

#include "bnChipUseListener.h"
#include "bnAnimatedTextBox.h"

<<<<<<< HEAD
class ChipDescriptionTextbox : public AnimatedTextBox {
public:
  ChipDescriptionTextbox(sf::Vector2f pos);
=======
/**
 * @class ChipDescriptionTextbox
 * @author mav
 * @date 05/05/19
 * @file bnChipDescriptionTextbox.h
 * @brief Opens textbox with navi's face and describes the chip
 */
class ChipDescriptionTextbox : public AnimatedTextBox {
public:
  /**
    * @brief sets the position of the textbox
    */
  ChipDescriptionTextbox(sf::Vector2f pos);
  
  /**
   * @brief Enqueues the chip description and plays it
   * @param chip the chip with information
   * 
   * If there's any messages left in the queue, dequeus them
   * before adding new information
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  void DescribeChip(Chip* chip);
};