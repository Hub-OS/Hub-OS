#include "bnAnimatedTextBox.h"
#include <cmath>

AnimatedTextBox::AnimatedTextBox(const sf::Vector2f& pos) :
  textArea(),
  totalTime(0)
{
  textureRef = Textures().LoadFromFile(TexturePaths::ANIMATED_TEXT_BOX);
  lastSpeaker = std::make_shared<SpriteProxyNode>();
  frame = std::make_shared<SpriteProxyNode>(*textureRef);
  textBox = std::make_shared<TextArea>(180, 22);

  // set the textbox positions
  setPosition(pos);
  setScale(2.0f, 2.0f);

  // Load the textbox animation
  animator = Animation(AnimationPaths::ANIMATED_TEXT_BOX);
  animator.Reload();

  isPaused = true;
  isReady = false;
  isOpening = false;
  isClosing = false;

  textBox->SetTextFillColor(sf::Color(66, 57, 57));

  AddNode(textBox);
  AddNode(frame);
  AddNode(lastSpeaker);

  textBox->setPosition(49.f, -22.f);
  textBox->Hide();
  lastSpeaker->setPosition(3.f, -23.f);
  lastSpeaker->Hide();
}

AnimatedTextBox::~AnimatedTextBox() { }

void AnimatedTextBox::Close() {
  if (!isReady || isClosing) return;

  lightenMug = true;
  isClosing = true;
  isReady = false;

  animator.SetAnimation("CLOSE");

  animator << Animator::On(2, [this] {
      textBox->Hide();
      lastSpeaker->Hide();
    }
  );

  auto callback = [this]() {
    isClosing = false;
    isOpening = false;
    isPaused = true;
  };

  animator << callback;
}


void AnimatedTextBox::Open(const std::function<void()>& onOpen) {
  if (isReady || isOpening) return;

  isOpening = true;

  animator.SetAnimation("OPEN");

  animator << Animator::On(3, [this] { 
    lightenMug = true; 
    textBox->Reveal();
    lastSpeaker->Reveal();
  });

  auto callback = [this, onOpen]() {
    isClosing = false;
    isPaused = false;
    isOpening = false;
    isReady = true;
    lightenMug = false;
    if (onOpen) onOpen();
  };

  animator << callback;
}

const bool AnimatedTextBox::IsPlaying() const { return !isPaused; }
const bool AnimatedTextBox::IsOpen() const { return isReady; }
const bool AnimatedTextBox::IsClosed() const { return !isReady && !isOpening && !isClosing; }

const bool AnimatedTextBox::HasMessage() {
  return (messages.size() > 0);
}

const bool AnimatedTextBox::IsEndOfMessage()
{
  return !textBox->HasMore() && IsEndOfBlock();
}

const bool AnimatedTextBox::IsEndOfBlock()
{
  return textBox->IsEndOfBlock();
}

bool AnimatedTextBox::IsFinalBlock() const {
  return textBox->IsFinalBlock();
}

void AnimatedTextBox::ShowPreviousLines()
{
  for (int i = 0; i < textBox->GetNumberOfFittingLines(); i++) {
    textBox->ShowPreviousLine();
  }

  isPaused = false;
}

void AnimatedTextBox::ShowNextLines()
{
  for (int i = 0; i < textBox->GetNumberOfFittingLines(); i++) {
    textBox->ShowNextLine();
  }

  isPaused = false;
}

const int AnimatedTextBox::GetNumberOfFittingLines() const
{
    return textBox->GetNumberOfFittingLines();
}

const float AnimatedTextBox::GetFrameWidth() const
{
  return frame->getLocalBounds().width;
}

const float AnimatedTextBox::GetFrameHeight() const
{
  return frame->getLocalBounds().height;
}

std::pair<size_t, size_t> AnimatedTextBox::GetCurrentCharacterRange() const
{
  return textBox->GetCurrentCharacterRange();
}

std::pair<size_t, size_t> AnimatedTextBox::GetCurrentLineRange() const
{
  return textBox->GetCurrentLineRange();
}

std::pair<size_t, size_t> AnimatedTextBox::GetBlockCharacterRange() const
{
  return textBox->GetBlockCharacterRange();
}

const int AnimatedTextBox::GetTextboxAreaWidth() const
{
  return textBox->GetAreaWidth();
}

const int AnimatedTextBox::GetTextboxAreaHeight() const
{
  return textBox->GetAreaHeight();
}

void AnimatedTextBox::CompleteCurrentBlock() {
  textBox->CompleteCurrentBlock();

  if (mugAnimator.GetAnimationString() != "IDLE") {
    mugAnimator.SetAnimation("IDLE");
    mugAnimator << Animator::Mode::Loop;
  }

  isPaused = false;
}

void AnimatedTextBox::DequeMessage() {
  if (messages.size() == 0) return;

  // We need an image of the last speaker when we close
  if (messages.size() == 1) {
    const sf::Texture* texture = mugshots.front().getTexture();

    if (texture) {
      lastSpeaker->setTexture(std::make_shared<sf::Texture>(*texture));
    }
    else {
      lastSpeaker->setTextureRect({});
    }
  }

  delete *messages.begin(); // TODO: use shared ptrs
  messages.erase(messages.begin());
  anims.erase(anims.begin());
  mugshots.erase(mugshots.begin());

  isPaused = false; // Begin playing again

  if (messages.size() == 0) return;

  // If we have a new speaker, use their image instead
  const sf::Texture* texture = mugshots.front().getTexture();

  if (texture) {
    lastSpeaker->setTexture(std::make_shared<sf::Texture>(*texture));
  }
  else {
    lastSpeaker->setTextureRect({});
  }

  mugAnimator = Animation(anims[0]);
  mugAnimator.SetAnimation("TALK");
  mugAnimator << Animator::Mode::Loop;
  textBox->SetText(messages[0]->GetMessage());
}

void AnimatedTextBox::ClearAllMessages()
{
  while (messages.size()) {
    DequeMessage();
  }
}
void AnimatedTextBox::EnqueMessage(const sf::Sprite& speaker, const Animation& anim, MessageInterface* message)
{
  messages.push_back(message);
  anims.push_back(anim);

  auto& mugAnim = anims[anims.size() - 1];
  mugAnim.SetAnimation("IDLE");
  mugAnim << Animator::Mode::Loop;

  mugshots.push_back(speaker);
  mugshots[mugshots.size() - 1].setScale(2.f, 2.f);

  if (messages.size() == 1) {
    const sf::Texture* texture = mugshots.front().getTexture();

    if (texture) {
      lastSpeaker->setTexture(std::make_shared<sf::Texture>(*texture));
    }
    else {
      lastSpeaker->setTextureRect({});
    }

    mugAnimator = mugAnim;
    textBox->SetText(message->GetMessage());
  }

  message->SetTextBox(this);
}

void AnimatedTextBox::EnqueMessage(MessageInterface* message) {
  EnqueMessage(sf::Sprite{}, Animation{}, message);
}

void AnimatedTextBox::ReplaceText(std::string text)
{
  textBox->SetText(text);
  isPaused = false; // start over with new text
}

void AnimatedTextBox::Update(double elapsed) {
  float mugshotSpeed = 1.0f;

  textBox->Update(elapsed * static_cast<float>(textSpeed));

  if (isReady && messages.size() > 0) {
    uint32_t currChar = textBox->GetCurrentCharacter();
    bool muteFX = (textBox->GetVFX() & TextArea::effects::zzz) == TextArea::effects::zzz;
    bool speakingDot = currChar == U'.' || currChar == U'\0';
    bool silence = (currChar == U' ' && mugAnimator.GetAnimationString() == "IDLE");
    bool lipsSealed = muteFX || speakingDot || silence;

    auto playIdleThunk = [this] {
      if (mugAnimator.GetAnimationString() != "IDLE") {
        mugAnimator.SetAnimation("IDLE");
        mugAnimator << Animator::Mode::Loop;
      }
    };

    if (!isPaused) {
      if (lipsSealed) {
        playIdleThunk();
      }
      else if (mugAnimator.GetAnimationString() != "TALK") {
        mugAnimator.SetAnimation("TALK");
        mugAnimator << Animator::Mode::Loop;
      }
      else {
        mugshotSpeed = static_cast<float>(textSpeed);
      }

      if (textBox->IsEndOfMessage() || textBox->HasMore()) {
        isPaused = true;
      }
    }
    else {
      playIdleThunk();
    }

    messages.front()->OnUpdate(elapsed);
  }

  if (mugshots.size()) {
    mugAnimator.Update(elapsed*mugshotSpeed, mugshots.front());
  }

  textBox->Play(!isPaused);
  animator.Update((float)elapsed, frame->getSprite());
  mugAnimator.Refresh(lastSpeaker->getSprite());

  if (isOpening || isReady || isClosing) {
    frame->Reveal();
  }
  else {
    frame->Hide();
  }

  if (lightenMug) {
    lastSpeaker->setColor(sf::Color(255, 255, 255, 125));
  }
  else {
    lastSpeaker->setColor(sf::Color::White);
  }
}

void AnimatedTextBox::SetTextSpeed(double factor) {
  textSpeed = std::max(1.0, factor);
}

void AnimatedTextBox::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
  states.transform *= this->getTransform();

  SceneNode::draw(target, states);
  
  if (messages.size() > 0 && isReady) {
    messages.front()->OnDraw(target, states);
  }
}

void AnimatedTextBox::DrawMessage(sf::RenderTarget & target, sf::RenderStates states) const
{
  target.draw(*textBox, states);
}

Text AnimatedTextBox::MakeTextObject(const std::string& data)
{
  Text obj = textBox->GetText();
  obj.SetString(data);
  return obj;
}

void AnimatedTextBox::ChangeAppearance(std::shared_ptr<sf::Texture> newTexture, const Animation& newAnimation)
{
  textureRef = newTexture;
  frame->setTexture(textureRef);
  animator = newAnimation;
  animator.Refresh(frame->getSprite());
}

void AnimatedTextBox::ChangeBlipSfx(std::shared_ptr<sf::SoundBuffer> newSfx)
{
  textBox->SetBlipSfx(newSfx);
}

Font AnimatedTextBox::GetFont() const {
  return textBox->GetText().GetFont();
}

sf::Vector2f AnimatedTextBox::GetTextPosition() const {
  return textBox->getPosition();
}

void AnimatedTextBox::Mute(bool enabled)
{
  textBox->Mute(enabled);
}

void AnimatedTextBox::Unmute() {
  textBox->Unmute();
}
