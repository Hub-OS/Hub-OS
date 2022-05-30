/*! \brief Displays the player's current emotion. */
#pragma once

#include <SFML/Graphics.hpp>
#include <memory>

#include "bnEmotions.h"
#include "bnFormSelectionListener.h"
#include "bnSpriteProxyNode.h"
#include "bnUIComponent.h"
#include "frame_time_t.h"

class Player;
class BattleSceneBase;

/** \brief Tracks the player's emotion per-frame. */
class PlayerEmotionUI : public UIComponent, public FormSelectionListener {
public:
  PlayerEmotionUI(std::weak_ptr<Player> player);
  ~PlayerEmotionUI();

  /** \brief Sets the texture used for emotions.
   *
   *  To use multiple emotions, the texture must contain 10 emotions (Emotions::COUNT).
   *  Emotions must be:
   *  - 16 pixels high each (full texture height == 160, recommended width == 43px)
   *  - stacked from top to bottom
   *  - in the order of the Emotion enum
   *
   *  Any smaller texture is interpreted as a single emotion texture.
   *  In this case, the emotion window will not change.
   */
  void SetTexture(std::shared_ptr<sf::Texture> texture);

  void Inject(BattleSceneBase& scene) override;

  /** \brief Draws the emotion window *if the player is not in a form* */
  void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

  /** \brief Updates the texture atlas cell if the emotion has changed.
   * Uses the
   */
  void OnUpdate(double elapsed) override;

  /**
   * @brief When a form is selected, hide the emotion window. If deselected, reveal.
   * @param index Index of selected player form, -1 if no form;
   */
  void OnFormSelected(int index) override;

  /** Height of a single emotion cell in the emotions altas texture, in pixels. @see SetTexture */
  static constexpr int AtlasCellHeight {16};
private:
  /** Sets the emotion visible in the Emotion Window */
  void SetDisplayedEmotion(Emotion emotion);

  bool isChangingEmotion{};
  std::shared_ptr<sf::Texture> texture;
  SpriteProxyNode emotionWindow;
  Emotion emotion; /**< Tracks the current player emotion */
  Emotion previousEmotion; /**< Holds the previous emotion so the emotion window knows what to flicker between on change */
  frame_time_t emotionChangeFrame; /**< Number of frames of the emotion flicker animation that have progressed */
};
