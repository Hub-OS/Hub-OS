#include "bnMessage.h"
#include "bnAnimatedTextBox.h"
#include "bnTextureResourceManager.h"
Message::Message(std::string message) : MessageInterface(message) {
  ResourceHandle handle;
  nextCursor.setTexture(handle.Textures().LoadFromFile(TexturePaths::TEXT_BOX_NEXT_CURSOR));
  totalElapsed = 0;
}

Message::~Message() { ; }

void Message::OnUpdate(double elapsed) {
  totalElapsed += elapsed;
}

void Message::OnDraw(sf::RenderTarget& target, sf::RenderStates states) {
  float bounce = std::sin((float)totalElapsed * 20.0f);

  nextCursor.setPosition(226, 15+bounce);

  GetTextBox()->DrawMessage(target, states);

  if (showEndMessageCursor && !GetTextBox()->IsPlaying() 
    && (GetTextBox()->IsEndOfMessage() || GetTextBox()->HasMessage())) {
    target.draw(nextCursor, states);
  }
}
void Message::SetTextBox(AnimatedTextBox * textbox)
{
  MessageInterface::SetTextBox(textbox);
}

Message::ContinueResult Message::Continue() {
  if (GetTextBox()->IsPlaying() || !GetTextBox()->HasMessage()) return ContinueResult::no_op;

  if (GetTextBox()->IsEndOfMessage()) {
    GetTextBox()->DequeMessage();
    return ContinueResult::dequeued;
  }
  
  // else
  GetTextBox()->ShowNextLines();
  return ContinueResult::next_lines;
}

void Message::ShowEndMessageCursor(bool show)
{
  showEndMessageCursor = show;
}

