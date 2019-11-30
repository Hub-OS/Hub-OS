#include "bnMessage.h"
#include "bnAnimatedTextBox.h"
#include "bnTextureResourceManager.h"
Message::Message(std::string message) : MessageInterface(message) {
  nextCursor = sf::Sprite(LOAD_TEXTURE(TEXT_BOX_NEXT_CURSOR));
  totalElapsed = 0;
}

Message::~Message() { ; }

void Message::OnUpdate(double elapsed) {
  totalElapsed += elapsed;
}

void Message::OnDraw(sf::RenderTarget& target, sf::RenderStates states) {
  auto bounce = std::sin((float)totalElapsed*10.0f)*1.0f;

  nextCursor.setPosition(sf::Vector2f(this->GetTextBox()->GetFrameWidth() - 20.0f, 10.0f + bounce));

  GetTextBox()->DrawMessage(target, states);

  if (!GetTextBox()->IsPlaying() && (GetTextBox()->IsEndOfMessage() || GetTextBox()->HasMessage())) {
    target.draw(nextCursor, states);
  }
}
void Message::SetTextBox(AnimatedTextBox * textbox)
{
  MessageInterface::SetTextBox(textbox);
}

void Message::Continue() {
  if (GetTextBox()->IsPlaying()) return;

  if (!GetTextBox()->IsEndOfMessage()) {
    if (GetTextBox()->HasMessage()) {
      GetTextBox()->DequeMessage();
    }
  }
  else {
    GetTextBox()->ShowNextLines();
  }
}

