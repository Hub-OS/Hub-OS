#pragma once

#include "bnCardUseListener.h"
#include "bnAnimatedTextBox.h"
#include "bnCard.h"
/**
 * @class CardDescriptionTextbox
 * @author mav
 * @date 05/05/19
 * @file bnCardDescriptionTextbox.h
 * @brief Opens textbox with navi's face and describes the card
 */
class CardDescriptionTextbox : public AnimatedTextBox {
public:
  /**
    * @brief sets the position of the textbox
    */
  CardDescriptionTextbox(const sf::Vector2f& pos);
  
  /**
   * @brief Enqueues the card description and plays it
   * @param card the card with information
   * 
   * If there's any messages left in the queue, dequeus them
   * before adding new information
   */
  void DescribeCard(Battle::Card* card);
};