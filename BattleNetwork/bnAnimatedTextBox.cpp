#include "bnAnimatedTextBox.h"
#include <cmath>

AnimatedTextBox::AnimatedTextBox(sf::Vector2f pos)
    : textArea(textArea), totalTime(0), textBox(280, 40, 24, "resources/fonts/NETNAVI_4-6_V3.ttf") {
    frame = sf::Sprite(LOAD_TEXTURE(ANIMATED_TEXT_BOX));
    nextCursor = sf::Sprite(LOAD_TEXTURE(TEXT_BOX_NEXT_CURSOR));
    selectCursor = sf::Sprite(LOAD_TEXTURE(TEXT_BOX_CURSOR));

    // set the textbox positions
    textBox.setPosition(sf::Vector2f(this->getPosition().x + 90.0f, this->getPosition().y - 40.0f));
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

AnimatedTextBox::~AnimatedTextBox() { }

void AnimatedTextBox::Close() {
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

const bool AnimatedTextBox::SelectYes() const {
  if (this->messages.size() == 0 || textBox.HasMore()) return false;
  if (!isReady || !this->messages[0]->IsQuestion()) return false;

  return this->messages[0]->SelectYes();
}

const bool AnimatedTextBox::SelectNo() const {
  if (this->messages.size() == 0 || textBox.HasMore()) return false;
  if (!isReady || !this->messages[0]->IsQuestion()) return false;

  return this->messages[0]->SelectNo();
}

const bool AnimatedTextBox::ConfirmSelection() {
  if (this->messages.size() == 0 || textBox.HasMore()) return false;
  if (!isReady || !this->messages[0]->IsQuestion()) return false;

  this->messages[0]->ExecuteSelection();
  this->DequeMessage();
  isPaused = false;

  return true;
}

void AnimatedTextBox::Open() {
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

const bool AnimatedTextBox::IsOpen() const { return isReady; }
const bool AnimatedTextBox::IsClosed() const { return !isReady; }

const bool AnimatedTextBox::HasMessage() {
  return (messages.size() > 0);
}

void AnimatedTextBox::DequeMessage() {
  if (messages.size() == 0) return;

  delete *messages.begin();
  messages.erase(messages.begin());
  animPaths.erase(animPaths.begin());
  mugshots.erase(mugshots.begin());

  if (messages.size() == 0) return;

  mugAnimator = Animation(animPaths[0]);
  mugAnimator.SetAnimation("TALK");
  mugAnimator << Animate::Mode::Loop;
  textBox.SetMessage(messages[0]->GetMessage());
}

void AnimatedTextBox::EnqueMessage(sf::Sprite speaker, std::string animationPath, Message* message) {
  speaker.setScale(2.0f, 2.0f);
  messages.push_back(message);

  animPaths.push_back(animationPath);
  mugshots.push_back(speaker);

  mugAnimator = Animation(animPaths[0]);
  mugAnimator.SetAnimation("TALK");
  mugAnimator << Animate::Mode::Loop;

  std::string strMessage = messages[0]->GetMessage();
  textBox.SetMessage(strMessage);
}

  void AnimatedTextBox::Update(float elapsed) {
  totalTime += elapsed;

  if (isReady && messages.size() > 0) {

    int yIndex = (int)(textBox.GetNumberOfLines() % textBox.GetNumberOfFittingLines());
    auto y = (textBox.GetNumberOfFittingLines() -yIndex) * 10.0f;
    y = frame.getPosition().y - y;

    if (this->messages[0]->IsYes()) {
      auto x = swoosh::ease::interpolate(elapsed * 10.f, selectCursor.getPosition().x, frame.getPosition().x + 140.0f);
      selectCursor.setPosition(x, y);
    }
    else {
      auto x = swoosh::ease::interpolate(elapsed * 10.f, selectCursor.getPosition().x, frame.getPosition().x + 265.0f);
      selectCursor.setPosition(x, y);
    }

    if (!isPaused) {
      if (mugAnimator.GetAnimationString() != "TALK") {
        mugAnimator.SetAnimation("TALK");
        mugAnimator << Animate::Mode::Loop;
      }

      textBox.Update(elapsed*(float)textSpeed);

      if (textBox.EndOfMessage() || textBox.HasMore()) {
        isPaused = true;
      }
    }
    else {
      if (mugAnimator.GetAnimationString() != "IDLE") {
        mugAnimator.SetAnimation("IDLE");
        mugAnimator << Animate::Mode::Loop;
      }
    }

    mugAnimator.Update(elapsed*(float)textSpeed, mugshots.front());
  }

  textBox.Play(!isPaused);

  // set the textbox position
  textBox.setPosition(sf::Vector2f(this->getPosition().x + 90.0f, this->getPosition().y - 40.0f));

  animator.Update(elapsed, frame);
}

void AnimatedTextBox::SetTextSpeed(double factor) {
  if (textSpeed >= 1.0) {
    textSpeed = factor;
  }
}

void AnimatedTextBox::Continue() {
  if (!this->isPaused) return;

  if (!textBox.HasMore()) {
    if (messages.size() > 1 && !this->messages[0]->IsQuestion()) {
      this->DequeMessage();
    }

  }
  else {
    for (int i = 0; i < textBox.GetNumberOfFittingLines(); i++) {
      textBox.ShowNextLine();
    }
  }

  isPaused = false;
}

void AnimatedTextBox::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
  frame.setScale(this->getScale()*2.0f);
  frame.setPosition(this->getPosition());
  frame.setRotation(this->getRotation());

  nextCursor.setScale(this->getScale()*2.0f);
  selectCursor.setScale(this->getScale()*2.0f);

  auto bounce = std::sin((float)totalTime*10.0f)*5.0f;

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

    pos += sf::Vector2f(6.0f, 2.0f - sprite.getGlobalBounds().height / 2.0f);

    sprite.setPosition(pos);

    mugAnimator.Update(0, sprite);

    target.draw(sprite);
    sprite.setPosition(oldpos);

    // Draw the animated text
    textBox.draw(target, states);

    if (this->messages[0]->IsQuestion() && !textBox.HasMore()) {

      // Draw the Yes / No and a cursor
      if (this->isPaused) {
        // TODO: draw yes / no?
        //sf::Text text = textBox.GetText();

        //target.draw(text);
        target.draw(selectCursor);
      }
    }
    else if (this->isPaused && (textBox.HasMore() || this->messages.size() > 1)) {
      target.draw(nextCursor);
    }
  }
}
