#include "bnOverworldMenuSystem.h"

#include "../bnAudioResourceManager.h"
#include "../bnMessage.h"
#include "../bnMessageQuestion.h"
#include "../bnMessageQuiz.h"

namespace Overworld {
  MenuSystem::MenuSystem() : textbox({ 4, 255 }), ResourceHandle() {}

  void MenuSystem::BindMenu(InputEvent event, std::shared_ptr<Menu> menu) {
    bindedMenus.push_back({ event, menu });
  }

  std::optional<std::reference_wrapper<BBS>> MenuSystem::GetBBS() {
    if (!pendingBbs.empty()) {
      return *pendingBbs.back().bbs;
    }

    if (!bbs.empty()) {
      return *bbs.back();
    }

    return {};
  }

  size_t MenuSystem::CountBBS() {
    return pendingBbs.size() + bbs.size();
  }

  void MenuSystem::ClearBBS() {
    while (!pendingBbs.empty()) {
      // todo: should this pop from back instead of front?
      pendingBbs.front().bbs->Close();
      pendingBbs.pop();
    }

    while (!bbs.empty()) {
      bbs.back()->Close();
    }

    bbs.clear();

    totalRemainingMessagesForBBS = 0;
    bbsNeedsAck = false;
  }

  void MenuSystem::EnqueueBBS(const std::string& topic, sf::Color color, const std::function<void(const std::string&)>& onSelect, const std::function<void()>& onClose) {
    auto remainingMessages = textbox.GetRemainingMessages();

    auto selectHandler = [this, onSelect](auto& selection) {
      bbsNeedsAck = true;
      onSelect(selection);
    };

    auto closeHandler = [this, onClose] {
      onClose();
      bbs.pop_back();
    };

    auto bbsPtr = std::make_unique<BBS>(topic, color, selectHandler, closeHandler);

    bbsPtr->setScale(2, 2);

    if (remainingMessages == 0) {
      bbs.push_back(std::move(bbsPtr));
      return;
    }

    remainingMessages -= totalRemainingMessagesForBBS;
    totalRemainingMessagesForBBS += remainingMessages;

    PendingBBS newPendingBbs = {
      std::move(bbsPtr),
      remainingMessages
    };

    pendingBbs.push(std::move(newPendingBbs));
  }

  void MenuSystem::AcknowledgeBBSSelection() {
    bbsNeedsAck = false;
  }

  void MenuSystem::SetNextSpeaker(const sf::Sprite& speaker, const Animation& animation) {
    textbox.SetNextSpeaker(speaker, animation);
  }

  void MenuSystem::PopMessage() {
    if (pendingBbs.empty()) {
      return;
    }

    totalRemainingMessagesForBBS -= 1;

    auto& [board, remainingMessages] = pendingBbs.front();

    remainingMessages -= 1;

    if (remainingMessages == 0) {
      bbs.push_back(std::move(board));
      pendingBbs.pop();
    }
  }

  void MenuSystem::EnqueueMessage(const std::string& message, const std::function<void()>& onComplete) {
    textbox.EnqueueMessage(message, [=] {
      PopMessage();
      onComplete();
    });
  }

  void MenuSystem::EnqueueQuestion(const std::string& prompt, const std::function<void(bool)>& onResponse) {
    textbox.EnqueueQuestion(prompt, [=](bool response) {
      PopMessage();
      onResponse(response);
    });
  }

  void MenuSystem::EnqueueQuiz(const std::string& optionA, const std::string& optionB, const std::string& optionC, const std::function<void(int)>& onResponse) {
    textbox.EnqueueQuiz(optionA, optionB, optionC, [=](int response) {
      PopMessage();
      onResponse(response);
    });
  }

  void MenuSystem::EnqueueTextInput(const std::string& initialText, size_t characterLimit, const std::function<void(const std::string&)>& onResponse) {
    textbox.EnqueueTextInput(initialText, characterLimit, [=](const std::string& text) {
      PopMessage();
      onResponse(text);
    });
  }

  bool MenuSystem::IsOpen() {
    return activeBindedMenu || textbox.IsOpen() || !bbs.empty();
  }

  bool MenuSystem::IsClosed() {
    return !activeBindedMenu && textbox.IsClosed() && bbs.empty();
  }

  bool MenuSystem::IsFullscreen() {
    return (activeBindedMenu && activeBindedMenu->IsFullscreen()) || GetBBS().has_value();
  }

  bool MenuSystem::ShouldLockInput() {
    return (activeBindedMenu && activeBindedMenu->LocksInput()) || textbox.IsOpen() || !bbs.empty();
  }

  void MenuSystem::Update(float elapsed) {
    if (activeBindedMenu) {
      activeBindedMenu->Update(elapsed);
    }

    if (!bbs.empty()) {
      bbs.back()->Update(elapsed);
    }

    textbox.Update(elapsed);
  }

  void MenuSystem::HandleInput(InputManager& input, sf::Vector2f mousePos) {
    if (activeBindedMenu) {
      // test to see if the menu closed itself or was closed externally
      if (activeBindedMenu->IsOpen()) {
        activeBindedMenu->HandleInput(input, mousePos);
        return;
      }
      else {
        activeBindedMenu = nullptr;
      }
    }

    if (textbox.GetRemainingMessages() > 0) {
      textbox.HandleInput(input, mousePos);
      return;
    }

    if (!bbs.empty()) {
      if (!bbsNeedsAck) {
        bbs.back()->HandleInput(input);
      }
      return;
    }

    // nothing else open, see if we should open a menu
    for (auto& [binding, menu] : bindedMenus) {
      if (input.Has(binding)) {
        menu->Open();
        activeBindedMenu = menu;
        return;
      }
    }
  }

  void MenuSystem::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (!bbs.empty()) {
      target.draw(*bbs.back(), states);
    }

    textbox.draw(target, states);

    if (activeBindedMenu) {
      activeBindedMenu->draw(target, states);
    }
  }
}
