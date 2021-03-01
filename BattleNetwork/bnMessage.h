#pragma once
#include "bnMessageInterface.h"
#include "bnSpriteProxyNode.h"
#include <string>

/**
   * @class Message
   * @author mav
   * @date 13/05/19
   * @brief Message object prints message in classic RPG style
   */
class Message : public MessageInterface {
  mutable SpriteProxyNode nextCursor; /*!< Green cursor at bottom-right */
  double totalElapsed;
  bool showEndMessageCursor{ false };
public:
  Message(std::string message);

  virtual ~Message();

  virtual void OnUpdate(double elapsed);
  virtual void OnDraw(sf::RenderTarget& target, sf::RenderStates states);
  virtual void SetTextBox(AnimatedTextBox* textbox);
  /**
  * @brief Continues the text if it didn't fit the line.
  *
  * If there are more messages and the current one is done printing, will deque
  * the message and begin the next one
  */
  void Continue();

  void ShowEndMessageCursor(bool show = true);
};