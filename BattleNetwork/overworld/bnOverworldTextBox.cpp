#include "bnOverworldTextBox.h"

#include "../bnAudioResourceManager.h"
#include "../bnMessage.h"
#include "../bnMessageQuestion.h"
#include "../bnMessageQuiz.h"
#include "../bnMessageInput.h"

namespace Overworld {
  TextArea::TextArea(sf::Vector2f pos) : textbox(pos), ResourceHandle() {
      turboScroll = false;
  }

  void TextArea::SetNextSpeaker(const sf::Sprite& speaker, const Animation& animation) {
    nextSpeaker = speaker;
    nextAnimation = animation;
  }

  void TextArea::EnqueueMessage(const std::string& message, const std::function<void()>& onComplete) {
    if (!textbox.HasMessage()) {
      textbox.Open();
    }

    Message* object = new Message(message);
    object->ShowEndMessageCursor(true);
    textbox.EnqueMessage(nextSpeaker, nextAnimation, object);

    handlerQueue.push([=](InputManager& input, sf::Vector2f mousePos) {
      if (input.Has(InputEvents::pressed_run)) {
          turboScroll = true;
          turboTimer = 0;
      }
      if (!input.Has(InputEvents::pressed_interact)) {
          if (!turboScroll || !input.Has(InputEvents::held_run)) {
              return;
          }
      }

      //only advance to next text window if interact is pressed, run is pressed, or run has been held for enough frames
      bool advance = false;
      turboTimer++;
      if (input.Has(InputEvents::pressed_interact) || input.Has(InputEvents::pressed_run) || turboTimer > 44)
      {
          advance = true;
          turboTimer = 0;
      }

      // continue the conversation if the text is complete
      if (textbox.IsEndOfMessage()) {
          if (advance) {
              onComplete();
              textbox.DequeMessage();
              handlerQueue.pop();
          }
      }
      else if (textbox.IsEndOfBlock()) {
          if (advance) {
              textbox.ShowNextLines();
          }
      }
      else {
          // double tapping talk or holding run will complete the block
          textbox.CompleteCurrentBlock();
      }
    });
  }

  void TextArea::EnqueueQuestion(const std::string& prompt, const std::function<void(bool)>& onResponse) {
    if (!textbox.HasMessage()) {
      textbox.Open();
    }

    auto question = new Question(prompt, [=]() {onResponse(1);}, [=]() {onResponse(0);});
    textbox.EnqueMessage(nextSpeaker, nextAnimation, question);


    handlerQueue.push([=](InputManager& input, sf::Vector2f mousePos) {
      bool left = input.Has(InputEvents::pressed_ui_left);
      bool right = input.Has(InputEvents::pressed_ui_right);
      bool confirm = input.Has(InputEvents::pressed_confirm);
      bool cancel = input.Has(InputEvents::pressed_cancel);

      if (left && right) {}
      else if (left) {
        question->SelectYes() ? Audio().Play(AudioType::CHIP_SELECT, AudioPriority::low) : 0;
      }
      else if (right) {
        question->SelectNo() ? Audio().Play(AudioType::CHIP_SELECT, AudioPriority::low) : 0;
      }

      if (!textbox.IsEndOfMessage()) {
        if (confirm && textbox.IsEndOfBlock()) {
            textbox.ShowNextLines();
        }
        else if ((confirm || cancel) && !textbox.IsEndOfBlock()) {
            textbox.CompleteCurrentBlock();
        }
      }
      else if (confirm && cancel) {}
      else if (confirm) {
        question->ConfirmSelection();
        handlerQueue.pop();
      }
      else if (cancel) {
        question->SelectNo();
        question->ConfirmSelection();
        handlerQueue.pop();
      }
    });
  }

  void TextArea::EnqueueQuiz(const std::string& optionA, const std::string& optionB, const std::string& optionC, const std::function<void(int)>& onResponse) {
    if (!textbox.HasMessage()) {
      textbox.Open();
    }

    auto quiz = new Quiz(optionA, optionB, optionC, onResponse);
    textbox.EnqueMessage(nextSpeaker, nextAnimation, quiz);

    handlerQueue.push([=](InputManager& input, sf::Vector2f mousePos) {
      bool up = input.Has(InputEvents::pressed_ui_up);
      bool down = input.Has(InputEvents::pressed_ui_down);
      bool confirm = input.Has(InputEvents::pressed_confirm);
      bool cancel = input.Has(InputEvents::pressed_cancel);

      if (up && down) { /* silence is golden */ }
      else if (up) {
        quiz->SelectUp() ? Audio().Play(AudioType::CHIP_SELECT, AudioPriority::low) : 0;
      }
      else if (down) {
        quiz->SelectDown() ? Audio().Play(AudioType::CHIP_SELECT, AudioPriority::low) : 0;
      }

      if (confirm) {
        if (!textbox.IsEndOfMessage()) {
          textbox.CompleteCurrentBlock();
        }
        else {
          quiz->ConfirmSelection();
          handlerQueue.pop();
        }
      }

      if (cancel && !textbox.IsEndOfMessage()) {
          textbox.CompleteCurrentBlock();
      }
    });
  }

  void TextArea::EnqueueTextInput(const std::string& initialText, size_t characterLimit, const std::function<void(const std::string&)>& onResponse) {
    if (!textbox.HasMessage()) {
      textbox.Open();
    }

    auto messageInput = new MessageInput(initialText, characterLimit);

    sf::Sprite sprite;
    Animation animation;
    textbox.EnqueMessage(sprite, animation, messageInput);

    handlerQueue.push([=](InputManager& input, sf::Vector2f mousePos) {
      if (input.HasFocus() && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
        messageInput->HandleClick(mousePos);
      }

      if (messageInput->IsDone()) {
        onResponse(messageInput->Submit());
        handlerQueue.pop();
      }
    });
  }

  bool TextArea::IsOpen() {
    return textbox.IsOpen();
  }

  bool TextArea::IsClosed() {
    return textbox.IsClosed();
  }

  size_t TextArea::GetRemainingMessages() {
    return handlerQueue.size();
  }

  void TextArea::Update(float elapsed) {
    textbox.Update(elapsed);
  }

  void TextArea::HandleInput(InputManager& input, sf::Vector2f mousePos) {
    if (!handlerQueue.empty()) {
      handlerQueue.front()(input, mousePos);
    }

    if (!textbox.HasMessage()) {
      // if there are no more messages, close
      turboScroll = false;
      textbox.Close();
    }
  }

  void TextArea::ChangeAppearance(std::shared_ptr<sf::Texture> newTexture, const Animation& newAnimation)
  {
    textbox.ChangeAppearance(newTexture, newAnimation);
  }

  void TextArea::ChangeBlipSfx(std::shared_ptr<sf::SoundBuffer> newSfx)
  {
    textbox.ChangeBlipSfx(newSfx);
  }

  void TextArea::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    textbox.draw(target, states);
  }
}
