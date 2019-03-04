#pragma once
#include "bnTextBox.h"
#include "bnAnimation.h"
#include <Swoosh/Ease.h>

class AnimatedTextBox : public sf::Drawable, public sf::Transformable {
public:
  class Message {
  protected:
    std::string message;
    bool isQuestion;
    bool yes;
    std::function<void()> onYes;
    std::function<void()> onNo;

  public:
    const std::string GetMessage() { return message; }
    const bool IsQuestion() { return isQuestion; }
    const bool SelectYes() { if (!isQuestion) return false; else yes = true; return true; }
    const bool SelectNo() { if (!isQuestion) return false; else yes = false; return true; }

    const bool IsYes() const { return yes; }
    void ExecuteSelection() {
      if (yes) {  onYes();  }
      else {
        { onNo(); }
      }
    }

    Message(std::string message) : message(message) {
      isQuestion = yes = false;
      //onNo = std::function<void()>();
      //onYes = std::function<void()>();
    }

    Message(const Message& rhs) {
      message = rhs.message;
      isQuestion = rhs.isQuestion;
      yes = rhs.yes;
      onYes = rhs.onYes;
      onNo = rhs.onNo;
    }

    virtual ~Message() { ;  }
  };

  class Question : public Message {
  public:
    Question(std::string message, std::function<void()> onYes = std::function<void()>(), std::function<void()> onNo = std::function<void()>()) : Message(message) {
      isQuestion = true; 
      this->onNo = onNo;
      this->onYes = onYes;
      this->message += std::string("\\nYes      No");
    }
  };

private:
  mutable sf::Sprite frame; // Size is inherited from this object 
  mutable sf::Sprite nextCursor; 
  mutable sf::Sprite selectCursor;

  mutable Animation mugAnimator;
  bool isPaused;
  bool isReady;
  bool isOpening;
  bool isClosing;
  mutable std::vector<sf::Sprite> mugshots;
  std::vector<std::string> animPaths;
  std::vector<Message*> messages;
  Animation animator;

  sf::IntRect textArea;
  TextBox textBox;

  double totalTime;
  double textSpeed;
public:
  AnimatedTextBox(sf::Vector2f pos);
  virtual ~AnimatedTextBox();

  void DequeMessage();
  void EnqueMessage(sf::Sprite speaker, std::string animationPath, Message* message);
  void Close();
  void Open();

  const bool SelectYes() const;
  const bool SelectNo() const;
  const bool ConfirmSelection();
  void Continue();

  const bool IsOpen() const;
  const bool IsClosed() const;
  const bool HasMessage();

  virtual void Update(float elapsed);
  void SetTextSpeed(double factor);
  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
};