#include "bnAnimatedTextBox.h"
#include <cmath>

AnimatedTextBox::AnimatedTextBox(const sf::Vector2f& pos) :
  totalTime(0)
{
  textureRef = Textures().LoadFromFile(TexturePaths::ANIMATED_TEXT_BOX);
  lastSpeaker = std::make_shared<SpriteProxyNode>();
  frame = std::make_shared<SpriteProxyNode>(*textureRef);
  textArea = std::make_shared<TextArea>(180, 22);

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

  textArea->SetTextFillColor(sf::Color(66, 57, 57));

  AddNode(textArea);
  AddNode(frame);
  AddNode(lastSpeaker);

  textArea->setPosition(49.f, -22.f);
  textArea->Hide();
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
      textArea->Hide();
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
    textArea->Reveal();
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
  return !textArea->HasMore() && IsEndOfBlock();
}

const bool AnimatedTextBox::IsEndOfBlock()
{
  return textArea->IsEndOfBlock();
}

const uint32_t AnimatedTextBox::GetCurrentCharacter() const
{
    return textArea->GetCurrentCharacter();
}

bool AnimatedTextBox::IsFinalBlock() const {
  return textArea->IsFinalBlock();
}

void AnimatedTextBox::ShowPreviousLines()
{
  for (int i = 0; i < textArea->GetNumberOfFittingLines(); i++) {
    textArea->ShowPreviousLine();
  }

  isPaused = false;
}

void AnimatedTextBox::ShowNextLines()
{
  for (int i = 0; i < textArea->GetNumberOfFittingLines(); i++) {
    textArea->ShowNextLine();
  }

  isPaused = false;
}

const int AnimatedTextBox::GetNumberOfFittingLines() const
{
    return textArea->GetNumberOfFittingLines();
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
  return textArea->GetCurrentCharacterRange();
}

std::pair<size_t, size_t> AnimatedTextBox::GetCurrentLineRange() const
{
  return textArea->GetCurrentLineRange();
}

std::pair<size_t, size_t> AnimatedTextBox::GetBlockCharacterRange() const
{
  return textArea->GetBlockCharacterRange();
}

const int AnimatedTextBox::GetTextboxAreaWidth() const
{
  return textArea->GetAreaWidth();
}

const int AnimatedTextBox::GetTextboxAreaHeight() const
{
  return textArea->GetAreaHeight();
}

void AnimatedTextBox::CompleteCurrentBlock() {
  textArea->CompleteCurrentBlock();

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
      mugAnimator.Refresh(lastSpeaker->getSprite());
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
    mugAnimator.Refresh(lastSpeaker->getSprite());
  }
  else {
    lastSpeaker->setTextureRect({});
  }

  mugAnimator = Animation(anims[0]);
  mugAnimator.SetAnimation("TALK");
  mugAnimator << Animator::Mode::Loop;
  textArea->SetText(messages[0]->GetMessage());
  mugAnimator.Refresh(lastSpeaker->getSprite());
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
    textArea->SetText(message->GetMessage());
  }

  message->SetTextBox(this);
}

void AnimatedTextBox::EnqueMessage(MessageInterface* message) {
  EnqueMessage(sf::Sprite{}, Animation{}, message);
}

void AnimatedTextBox::ReplaceText(std::string text)
{
  textArea->SetText(text);
  isPaused = false; // start over with new text
}

void AnimatedTextBox::Update(double elapsed) {
  float mugshotSpeed = 1.0f;

  textArea->Update(elapsed * static_cast<float>(textSpeed));

  if (isReady && messages.size() > 0) {
    uint32_t currChar = textArea->GetCurrentCharacter();
    bool muteFX = (textArea->GetVFX() & TextArea::effects::zzz) == TextArea::effects::zzz;
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

      if (textArea->IsEndOfMessage() || textArea->HasMore()) {
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

  textArea->Play(!isPaused);
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
  target.draw(*textArea, states);
}

Text AnimatedTextBox::MakeTextObject(const std::string& data)
{
  Text obj = textArea->GetText();
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
  textArea->SetBlipSfx(newSfx);
}

Font AnimatedTextBox::GetFont() const {
  return textArea->GetText().GetFont();
}

sf::Vector2f AnimatedTextBox::GetTextPosition() const {
  return textArea->getPosition();
}

void AnimatedTextBox::Mute(bool enabled)
{
  textArea->Mute(enabled);
}

void AnimatedTextBox::Unmute() {
  textArea->Unmute();
}
