#pragma once
#include "bnTextBox.h"
#include "bnAnimation.h"

class AnimatedTextBox : public sf::Drawable, public sf::Transformable {
private:
  mutable sf::Sprite frame; // Size is inherited from this object 
  bool isPaused;
  bool isReady;
  bool isOpening;
  bool isClosing;
  mutable std::vector<sf::Sprite> mugshots;
  std::vector<std::string> animPaths;
  std::vector<std::string> messages;
  Animation animator;
  mutable Animation mugAnimator;

  sf::IntRect textArea;
  TextBox textBox;

public:
  AnimatedTextBox(sf::Vector2f pos, sf::IntRect textArea, int characterSize = 30, 
                  std::string fontPath = "resources/fonts/mmbnthin_regular.ttf") 
    : textArea(textArea), textBox(textArea.width, textArea.height, characterSize, fontPath) {
    frame = sf::Sprite(LOAD_TEXTURE(ANIMATED_TEXT_BOX));
    this->setPosition(pos);

    // set the textbox position
    textBox.setPosition(sf::Vector2f(this->getPosition().x + textArea.left, this->getPosition().y + textArea.top));

    // Load the textbox animation
    animator = Animation("resources/ui/textbox.animation");
    animator.Reload();

    isPaused = true;
    isReady = false;
    isOpening = false;
    isClosing = false;

    textBox.SetTextFillColor(sf::Color::Black);
  }

  virtual ~AnimatedTextBox() { }

  void Close() {
    if (!isReady || isClosing) return;

    isClosing = true;
    isReady = false;

    animator.SetAnimation("CLOSE");

    auto callback = [this]() {
      this->isClosing = false;
      this->isOpening = false;
      this->isPaused = true;
    };

    animator << Animate::On(2, callback, true);
  }

  void Open() {
    if (isReady || isOpening) return;

    isOpening = true;

    animator.SetAnimation("OPEN");

    auto callback = [this]() {
      this->isPaused = false;
      this->isOpening = false;
      this->isReady = true;
    };

    animator << Animate::On(2, callback, true);
  }

  const bool IsOpen() const { return isReady;  }
  const bool IsClosed() const { return !isReady; }

  void DequeMessage() {
    if (messages.size() == 0) return;
    messages.erase(messages.begin());
    animPaths.erase(animPaths.begin());
    mugshots.erase(mugshots.begin());
  }
  
  void EnqueMessage(sf::Sprite speaker, std::string animationPath, std::string message) {
    speaker.setScale(2.0f, 2.0f);
    messages.push_back(message);
    animPaths.push_back(animationPath);
    mugshots.push_back(speaker);

    mugAnimator = Animation(animPaths[0]);
    mugAnimator.SetAnimation("TALK");
    mugAnimator << Animate::Mode(Animate::Mode::Loop);
    textBox.SetMessage(messages[0]);
  }

  virtual void Update(float elapsed) {
    if (isReady && messages.size() > 0) {
      if (!isPaused) {
        if (mugAnimator.GetAnimationString() != "TALK") {
          mugAnimator.SetAnimation("TALK");
          mugAnimator << Animate::Mode(Animate::Mode::Loop);
        }

        textBox.Update(elapsed);

        if (textBox.EndOfMessage()) {
          isPaused = true;
        }
      }
      else {
        if (mugAnimator.GetAnimationString() != "IDLE") {
          mugAnimator.SetAnimation("IDLE");
          mugAnimator << Animate::Mode(Animate::Mode::Loop);
        }
      }

      mugAnimator.Update(elapsed, &mugshots.front());
    }

    animator.Update(elapsed, &frame);
  }

  void Continue() {
    if (!textBox.HasMore()) {
      if (messages.size() > 1) {
        messages.erase(messages.begin());
        mugAnimator = Animation(animPaths[0]);
        mugAnimator.SetAnimation("TALK");
        textBox.SetMessage(messages[0]);
        isPaused = false;
      }
    }
    else {
      textBox.ShowNextLine();
      textBox.ShowNextLine();
      isPaused = false;
    }
  }

  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const
  {
    frame.setScale(this->getScale()*2.0f);
    frame.setPosition(this->getPosition());
    frame.setRotation(this->getRotation());

    if (isOpening || isReady || isClosing) {
      target.draw(frame);
    }

    if (messages.size() > 0 && isReady) {
      sf::Sprite& sprite = mugshots.front();

      sf::Vector2f oldpos = sprite.getPosition();
      auto pos = oldpos;
      pos += this->getPosition();

      // This is a really bad design hack
      // I inherited the text area class which uses it's position as the text
      // starting point. But the textbox does NOT have text on the left-hand side
      // and it begins text offset to the right.
      // So we store the state of the object and then calculate the offset, and then
      // restore it when we draw
      // Prime example where scene nodes would come in handy.

      pos += sf::Vector2f(6.0f, -sprite.getGlobalBounds().height-4.0f);

      sprite.setPosition(pos);

      mugAnimator.Update(0, &sprite);

      target.draw(sprite);
      sprite.setPosition(oldpos);

      // Draw the animated text
      textBox.draw(target, states);
    }
  }
};