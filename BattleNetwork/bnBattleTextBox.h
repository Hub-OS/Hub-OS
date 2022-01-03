#pragma once
#include "bnResourceHandle.h"
#include "bnInputManager.h"
#include "bnAnimatedTextBox.h"
#include "bnCard.h"
/**
 * @class BattleTextBox
 * @author mav
 * @date 05/05/19
 * @file bnBattleTextBox.h
 * @brief Opens textbox with navi's face and describes the card. Used for retreating as well.
 */

class Question;

namespace Battle {
  class TextBox : public AnimatedTextBox {
    bool requestedRetreat{}, asking{};
    sf::Sprite mug;
    Animation anim;
    Question* question{ nullptr };
  public:
    /**
      * @brief sets the position of the textbox
      */
    TextBox(const sf::Vector2f& pos);

    /**
     * @brief Enqueues the card description and plays it
     * @param card the card with information
     *
     * If there's any messages left in the queue, dequeus them
     * before adding new information
     */
    void DescribeCard(Battle::Card* card);
    void PromptRetreat();
    void SetSpeaker(const sf::Sprite& mug, const Animation& anim);
    void Reset();

    const bool RequestedRetreat() const;
    const bool HasQuestion() const;
    bool SelectYes() const;
    bool SelectNo() const;
    void ConfirmSelection();
    void Cancel();
  };
}