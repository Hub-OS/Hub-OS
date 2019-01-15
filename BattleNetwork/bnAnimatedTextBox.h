#pragma once
#include "bnTextBox.h"
#include "bnAnimation.h"

class AnimatedTextBox : public sf::Drawable, public sf::Transformable {
private:
  mutable sf::Sprite frame; // Size is inherited from this object 
  mutable sf::Sprite nextCursor; 
  mutable Animation mugAnimator;
  bool isPaused;
  bool isReady;
  bool isOpening;
  bool isClosing;
  mutable std::vector<sf::Sprite> mugshots;
  std::vector<std::string> animPaths;
  std::vector<std::string> messages;
  Animation animator;

  sf::IntRect textArea;
  TextBox textBox;

  double totalTime;
  double textSpeed;
public:
  AnimatedTextBox(sf::Vector2f pos) 
    : textArea(textArea), totalTime(0), textBox(280, 40, 24, "resources/fonts/NETNAVI_4-6_V3.ttf") {
    frame = sf::Sprite(LOAD_TEXTURE(ANIMATED_TEXT_BOX));
    nextCursor = sf::Sprite(LOAD_TEXTURE(TEXT_BOX_NEXT_CURSOR));

    // set the textbox positions
    textBox.setPosition(sf::Vector2f(this->getPosition().x + 90.0, this->getPosition().y - 40.0));
    this->setPosition(pos);

    textSpeed = 1.0;

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

    animator << Animate::On(3, callback, true);
  }

  void Open() {
    if (isReady || isOpening) return;

    isOpening = true;

    animator.SetAnimation("OPEN");

    auto callback = [this]() {
      this->isClosing = false;
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
    totalTime += elapsed;

    if (isReady && messages.size() > 0) {
      if (!isPaused) {
        if (mugAnimator.GetAnimationString() != "TALK") {
          mugAnimator.SetAnimation("TALK");
          mugAnimator << Animate::Mode(Animate::Mode::Loop);
        }

        textBox.Update(elapsed*(float)textSpeed);

        if (textBox.EndOfMessage() || textBox.HasMore()) {
          isPaused = true;
        }
      }
      else {
        if (mugAnimator.GetAnimationString() != "IDLE") {
          mugAnimator.SetAnimation("IDLE");
          mugAnimator << Animate::Mode(Animate::Mode::Loop);
        }
      }

      mugAnimator.Update(elapsed*(float)textSpeed, &mugshots.front());
    }

    textBox.Play(!isPaused);

    // set the textbox position
    textBox.setPosition(sf::Vector2f(this->getPosition().x + 90.0, this->getPosition().y - 40.0));

    animator.Update(elapsed, &frame);
  }

  void SetTextSpeed(double factor) {
    if (textSpeed >= 1.0) {
      textSpeed = factor;
    }
  }

  void Continue() {
    if (!this->isPaused) return;

    if (!textBox.HasMore()) {
      if (messages.size() > 1) {
        messages.erase(messages.begin());
        animPaths.erase(animPaths.begin());
        mugshots.erase(mugshots.begin());

        mugAnimator = Animation(animPaths[0]);
        mugAnimator.SetAnimation("TALK");
        mugAnimator << Animate::Mode(Animate::Mode::Loop);
        textBox.SetMessage(messages[0]);
        isPaused = false;
      }
    }
    else {
      for (int i = 0; i < textBox.GetNumberOfFittingLines(); i++) {
        textBox.ShowNextLine();
      }

      isPaused = false;
    }
  }

  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const
  {
    frame.setScale(this->getScale()*2.0f);
    frame.setPosition(this->getPosition());
    frame.setRotation(this->getRotation());

    nextCursor.setScale(this->getScale()*2.0f);

    auto bounce = std::sinf((float)totalTime*10.0f)*5.0f;

    nextCursor.setPosition(sf::Vector2f(this->getPosition().x + frame.getGlobalBounds().width - 30.0f, this->getPosition().y + 30.0f + bounce));
    nextCursor.setRotation(this->getRotation());

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

      pos += sf::Vector2f(6.0f, 2.0f-sprite.getGlobalBounds().height/2.0f);

      sprite.setPosition(pos);

      mugAnimator.Update(0, &sprite);

      target.draw(sprite);
      sprite.setPosition(oldpos);

      // Draw the animated text
      textBox.draw(target, states);

      if (this->isPaused && (textBox.HasMore() || this->messages.size() > 1)) {
        target.draw(nextCursor);
      }
    }
  }
};