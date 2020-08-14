#pragma once

#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <array>
#include <memory>

class Mob;
class BattleItem;

/**
 * @class BattleResults
 * @author mav
 * @date 13/05/19
 * @brief BattleResults modal that appears at the end of the BattleScene when Player wins
 *
 * Exposes an API to interact with the modal
 */
class BattleResults {
private:
  sf::Sprite resultsSprite; /*!< This modals graphic */
  sf::Text time; /*!< Formatted time label */
  sf::Text rank; /*!< Battle scored rank */
  sf::Text reward; /*!< Name of reward */
  sf::Text cardCode; /*!< Code for cards */
  sf::Sprite rewardCard; /*!< Reward card graphics */
  sf::Sprite pressA; /*!< Press A sprite */
  sf::Sprite star; /*!< Counter stars */
  std::shared_ptr<sf::Font> font;

  bool isRevealed; /*!< Flag if modal is revealed */
  bool playSoundOnce; /*!< Flag to play sounds once */
  bool rewardIsCard; /*!< Is current reward a card */

  BattleItem* item; /*!< The item stored in this modal */
  int score; /*!< 1-10 or 11+ as S rank */
  int counterCount; /*!< How many times player countered */

  double totalElapsed; /*!< delta time this frame */

  std::array<int, 7*6> hideCardMatrix; /*~< blocks are 7x6 block space to uncover at 8x8 pixels*/
  int cardMatrixIndex;

  /**
   * @brief Format the time to look like BN time stamp
   * @param time
   * @return formatted string
   */
  std::string FormatString(sf::Time time);

public:
  /**
   * @brief Uses MMBN6 algorithm to score player based on various moves
   * @param battleLength duration of the battle
   * @param moveCount how many times player moved
   * @param hitCount how many times player got hit
   * @param counterCount how many time player countered enemies
   * @param doubleDelete if player double deleted enemies
   * @param tripleDelete if player triple deleted enemies
   * @param mob extra mob information e.g. if considered boss mob or common mob
   */
  BattleResults(sf::Time battleLength, int moveCount, int hitCount, int counterCount, bool doubleDelete, bool tripleDelete, Mob* mob);
  ~BattleResults();

  /**
   * @brief Confirms actions
   * @return true if successful, false otherwise
   */
  bool CursorAction();

  /**
   * @brief Query if modal is out of view
   * @return true if out of view at target destination, false otherwise
   */
  bool IsOutOfView();

  /**
   * @brief Query if modal is in view
   * @return true if in view at target destination, false otherwise
   */
  bool IsInView();

  /**
   * @brief Offset the modal on screen by delta pixels
   * @param delta
   */
  void Move(sf::Vector2f delta);

  /**
   * @brief Update modal and animations
   * @param elapsed in seconds
   */
  void Update(double elapsed);

  /**
   * @brief Perform draw steps
   */
  void Draw();

  /**
   * @brief Query if BattleRewards modal has completed all rewards
   * @return true if no more rewards, false if there are more the player needs to retrieve
   */
  bool IsFinished();

  /**
   * @brief Get the current reward item
   * @return BattleItem* if an item exists, nullptr if no reward was granted
   *
   * Will update the internal pointer to the next reward or nullptr if no more rewards
   */
  BattleItem* GetReward();
};

