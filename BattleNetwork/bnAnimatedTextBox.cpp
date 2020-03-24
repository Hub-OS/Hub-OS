#include "bnAnimatedTextBox.h"
#include <cmath>

AnimatedTextBox::AnimatedTextBox(sf::Vector2f pos)
    : textArea(), totalTime(0), textBox(280, 40, 24, "resources/fonts/NETNAVI_4-6_V3.ttf") {
    textureRef = LOAD_TEXTURE(ANIMATED_TEXT_BOX);
    frame = sf::Sprite(*textureRef);

    // set the textbox positions
    textBox.setPosition(sf::Vector2f(this->getPosition().x + 90.0f, this->getPosition().y - 40.0f));
    this->setPosition(pos);
    this->setScale(2.0f, 2.0f);

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

  animator << callback;
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

  animator << callback;
}

const bool AnimatedTextBox::IsPlaying() const { return !isPaused; }
const bool AnimatedTextBox::IsOpen() const { return isReady; }
const bool AnimatedTextBox::IsClosed() const { return !isReady; }

const bool AnimatedTextBox::HasMessage() {
  return (messages.size() > 0);
}

const bool AnimatedTextBox::IsEndOfMessage()
{
  return !textBox.HasMore();
}

void AnimatedTextBox::ShowNextLines()
{
  for (int i = 0; i < textBox.GetNumberOfFittingLines(); i++) {
    textBox.ShowNextLine();
  }

  isPaused = false;
}

const int AnimatedTextBox::GetNumberOfFittingLines() const
{
    return textBox.GetNumberOfFittingLines();
}

const float AnimatedTextBox::GetFrameWidth() const
{
  return frame.getLocalBounds().width;
}

const float AnimatedTextBox::GetFrameHeight() const
{
  return frame.getLocalBounds().height;
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
  mugAnimator << Animator::Mode::Loop;
  textBox.SetText(messages[0]->GetMessage());

  isPaused = false; // Begin playing again
}

void AnimatedTextBox::EnqueMessage(sf::Sprite speaker, std::string animationPath, MessageInterface* message) {
  speaker.setScale(2.0f, 2.0f);
  messages.push_back(message);

  animPaths.push_back(animationPath);
  mugshots.push_back(speaker);

  mugAnimator = Animation(animPaths[0]);
  mugAnimator.SetAnimation("TALK");
  mugAnimator << Animator::Mode::Loop;

  std::string strMessage = messages[0]->GetMessage();
  textBox.SetText(strMessage);

  message->SetTextBox(this);
}

void AnimatedTextBox::ReplaceText(std::string text)
{
    textBox.SetText(text);
    this->isPaused = false; // start over with new text
}

  void AnimatedTextBox::Update(double elapsed) {
  totalTime += elapsed;

  if (isReady && messages.size() > 0) {

    int yIndex = (int)(textBox.GetNumberOfLines() % textBox.GetNumberOfFittingLines());
    auto y = (textBox.GetNumberOfFittingLines() -yIndex) * 10.0f;
    y = frame.getPosition().y - y;

    if (!isPaused) {
      if (mugAnimator.GetAnimationString() != "TALK") {
        mugAnimator.SetAnimation("TALK");
        mugAnimator << Animator::Mode::Loop;
      }

      textBox.Update(elapsed*(float)textSpeed);

      if (textBox.EndOfMessage() || textBox.HasMore()) {
        isPaused = true;
      }
    }
    else {
      if (mugAnimator.GetAnimationString() != "IDLE") {
        mugAnimator.SetAnimation("IDLE");
        mugAnimator << Animator::Mode::Loop;
      }
    }

    mugAnimator.Update((float)(elapsed*textSpeed), mugshots.front());

    messages.front()->OnUpdate(elapsed);
  }

  textBox.Play(!isPaused);

  // set the textbox position
  textBox.setPosition(sf::Vector2f(this->getPosition().x + 90.0f, this->getPosition().y - 40.0f));

  animator.Update((float)elapsed, frame);
}

void AnimatedTextBox::SetTextSpeed(double factor) {
  if (textSpeed >= 1.0) {
    textSpeed = factor;
  }
}

void AnimatedTextBox::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
  frame.setScale(this->getScale());
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

    pos += sf::Vector2f(6.0f, 2.0f - sprite.getGlobalBounds().height / 2.0f);

    sprite.setPosition(pos);

    mugAnimator.Update(0, sprite);

    if (IsOpen()) {
      target.draw(sprite);
      sprite.setPosition(oldpos);
    }

    states.transform = this->getTransform();

    messages.front()->OnDraw(target, states);
  }
}

void AnimatedTextBox::DrawMessage(sf::RenderTarget & target, sf::RenderStates states) const
{
  target.draw(textBox, states);
}

sf::Text AnimatedTextBox::MakeTextObject(std::string data)
{
  sf::Text obj = textBox.GetText();
  obj.setFont(textBox.GetFont());
  obj.setString(data);

  return obj;
}
