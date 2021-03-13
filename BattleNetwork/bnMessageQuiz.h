#pragma once
#include "bnSpriteProxyNode.h"
#include "bnMessageInterface.h"
#include "bnAnimatedTextBox.h"
#include <string>
#include <functional>
#include <array>

/**
  * @class Quiz
  * @author konst
  * @date 07/03/21
  * @brief Initializes yes/no message objects
  *
  * 
  */
class Quiz : public MessageInterface {
private:
  std::function<void(int)> onResponse; /*!< Callback when user responds 0-2 */
  mutable SpriteProxyNode selectCursor; /*!< Used for making selections */
  double elapsed;
  int selection;
  std::array<std::string, 3> options;
public:
  Quiz(const std::string& optionA, const std::string& optionB, const std::string& optionC, const std::function<void(int)>& onResponse = [](int){});

  /**
   * @brief Moves selection up if applicable
   * @return true if success, false otherwise
   */
  const bool SelectUp();

  /**
   * @brief Moves selection down if applicable
   * @return true if success, false otherwise
   */
  const bool SelectDown();

  void ConfirmSelection();

  void OnUpdate(double elapsed) override;
  void OnDraw(sf::RenderTarget& target, sf::RenderStates states) override;
};
