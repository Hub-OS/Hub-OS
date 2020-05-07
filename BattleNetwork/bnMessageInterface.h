#pragma once
#include <SFML/Graphics.hpp>

class AnimatedTextBox;

class MessageInterface {
private:
  std::string message;
  AnimatedTextBox* textbox;

public:
  MessageInterface(std::string message, AnimatedTextBox* parent = nullptr) : message(message) { textbox = parent; }
  virtual void SetTextBox(AnimatedTextBox* textbox) { MessageInterface::textbox = textbox;  }
  AnimatedTextBox* GetTextBox() const { return textbox; }
  virtual ~MessageInterface() { ; }
  const std::string GetMessage() { return message; }
  virtual void OnUpdate(double elapsed) = 0;
  virtual void OnDraw(sf::RenderTarget& target, sf::RenderStates states) = 0;
};