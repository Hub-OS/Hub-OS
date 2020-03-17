#pragma once
#include "bnMessage.h"
#include "bnAnimatedTextBox.h"
#include <string>
#include <functional>

/**
    * @class Question
   * @author mav
   * @date 13/05/19
   * @brief Initializes yes/no message objects
   *
   * @warning honestly this design could be changed to do without the hackery and should see changes
   */
class Question : public Message {
private:
  bool yes; /*!< Flag for if yes was selected */
  bool canceled;
  std::function<void()> onYes; /*!< Callback when user presses yes */
  std::function<void()> onNo; /*!< Callback when user presses no */
  bool isQuestionReady; /*!< Flag for when the user has been prompted and input is waiting */
  mutable SpriteProxyNode selectCursor; /*!< Used for making selections */
  double elapsed;
  sf::Text options;
public:
  inline static const std::function<void()> NoCallback = []() {};

  Question(std::string message, std::function<void()> onYes = Question::NoCallback, std::function<void()> onNo = Question::NoCallback);
  ~Question();

  /**
  * @brief Selects yes if applicable
  * @return true if success, false otherwise
  */
  const bool SelectYes();

  /**
 * @brief Selects no if applicable
 * @return true if success, false otherwise
 */
  const bool SelectNo();

  void ExecuteSelection();

  void OnUpdate(double elapsed);
  void OnDraw(sf::RenderTarget& target, sf::RenderStates states);

  void SetTextBox(AnimatedTextBox* parent);

};
