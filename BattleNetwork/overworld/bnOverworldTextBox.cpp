#include "bnOverworldTextBox.h"

#include "../bnAudioResourceManager.h"
#include "../bnMessage.h"
#include "../bnMessageQuestion.h"
#include "../bnMessageQuiz.h"
#include "bnOverworldMessageInput.h"

namespace Overworld {
  TextBox::TextBox(sf::Vector2f pos) : textbox(pos), ResourceHandle() {}

  void TextBox::SetNextSpeaker(const sf::Sprite& speaker, const Animation& animation) {
    nextSpeaker = speaker;
    nextAnimation = animation;
  }

  void TextBox::EnqueueMessage(const std::string& message, const std::function<void()>& onComplete) {
    if (!textbox.HasMessage()) {
      textbox.Open();
    }

    Message* object = new Message(message);
    object->ShowEndMessageCursor(true);
    textbox.EnqueMessage(nextSpeaker, nextAnimation, object);

    handlerQueue.push([=](InputManager& input, const sf::RenderWindow& window) {
      if (!input.Has(InputEvents::pressed_interact)) {
        return;
      }

      // continue the conversation if the text is complete
      if (textbox.IsEndOfMessage()) {
        onComplete();
        textbox.DequeMessage();
        handlerQueue.pop();
      }
      else if (textbox.IsEndOfBlock()) {
        textbox.ShowNextLines();
      }
      else {
        // double tapping talk will complete the block
        textbox.CompleteCurrentBlock();
      }
    });
  }

  void TextBox::EnqueueQuestion(const std::string& prompt, const std::function<void(bool)>& onResponse) {
    if (!textbox.HasMessage()) {
      textbox.Open();
    }

    auto question = new Question(prompt, [=]() {onResponse(1);}, [=]() {onResponse(0);});
    textbox.EnqueMessage(nextSpeaker, nextAnimation, question);


    handlerQueue.push([=](InputManager& input, const sf::RenderWindow& window) {
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
        if (confirm) {
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

  void TextBox::EnqueueQuiz(const std::string& optionA, const std::string& optionB, const std::string& optionC, const std::function<void(int)>& onResponse) {
    if (!textbox.HasMessage()) {
      textbox.Open();
    }

    auto quiz = new Quiz(optionA, optionB, optionC, onResponse);
    textbox.EnqueMessage(nextSpeaker, nextAnimation, quiz);

    handlerQueue.push([=](InputManager& input, const sf::RenderWindow& window) {
      bool up = input.Has(InputEvents::pressed_ui_up);
      bool down = input.Has(InputEvents::pressed_ui_down);
      bool confirm = input.Has(InputEvents::pressed_confirm);

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
    });
  }

  void TextBox::EnqueueTextInput(const std::string& initialText, size_t characterLimit, const std::function<void(const std::string&)>& onResponse) {
    if (!textbox.HasMessage()) {
      textbox.Open();
    }

    auto messageInput = new MessageInput(initialText, characterLimit);

    sf::Sprite sprite;
    Animation animation;
    textbox.EnqueMessage(sprite, animation, messageInput);

    handlerQueue.push([=](InputManager& input, const sf::RenderWindow& window) {
      if (input.HasFocus() && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
        auto mousei = sf::Mouse::getPosition(window);
        auto mousef = window.mapPixelToCoords(mousei);
        messageInput->HandleClick(mousef);
      }

      if (messageInput->IsDone()) {
        onResponse(messageInput->Submit());
        handlerQueue.pop();
      }
    });
  }

  bool TextBox::IsOpen() {
    return textbox.IsOpen();
  }

  bool TextBox::IsClosed() {
    return textbox.IsClosed();
  }

  size_t TextBox::GetRemainingMessages() {
    return handlerQueue.size();
  }

  void TextBox::Update(float elapsed) {
    textbox.Update(elapsed);
  }

  void TextBox::HandleInput(InputManager& input, const sf::RenderWindow& window) {
    if (!handlerQueue.empty()) {
      handlerQueue.front()(input, window);
    }

    if (!textbox.HasMessage()) {
      // if there are no more messages, close
      textbox.Close();
    }
  }

  void TextBox::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    textbox.draw(target, states);
  }
}
