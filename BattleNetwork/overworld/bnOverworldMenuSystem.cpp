#include "bnOverworldMenuSystem.h"

#include "../bnAudioResourceManager.h"
#include "../bnMessage.h"
#include "../bnMessageQuestion.h"
#include "../bnMessageQuiz.h"

constexpr float BBS_FADE_DURATION_MAX = 0.5;

namespace Overworld {
  MenuSystem::MenuSystem() : textbox({ 4, 255 }), ResourceHandle() {}

  void MenuSystem::BindMenu(InputEvent event, std::shared_ptr<Menu> menu) {
    bindedMenus.push_back({ event, menu });
  }

  std::optional<std::reference_wrapper<BBS>> MenuSystem::GetBBS() {
    if (bbs) {
      return *bbs;
    }

    return {};
  }

  void MenuSystem::CloseBBS() {
    if (bbs) {
      bbs->Close();
      bbs = nullptr;
    }
  }

  void MenuSystem::OpenBBS(const std::string& topic, sf::Color color, const std::function<void(const std::string&)>& onSelect, const std::function<void()>& onClose) {
    if (bbs) {
      bbs->Close();
    }

    auto selectHandler = [this, onSelect](auto& selection) {
      bbsNeedsAck = true;
      onSelect(selection);
    };

    auto closeHandler = [this, onClose] {
      onClose();
      closingBbs = std::move(bbs);
      bbsFadeDuration = 0.0f;
    };

    bbs = std::make_unique<BBS>(topic, color, selectHandler, closeHandler);
    bbs->setScale(2, 2);

    bbsFadeDuration = 0.0f;
  }

  void MenuSystem::AcknowledgeBBSSelection() {
    bbsNeedsAck = false;
  }

  void MenuSystem::SetNextSpeaker(const sf::Sprite& speaker, const Animation& animation) {
    textbox.SetNextSpeaker(speaker, animation);
  }

  void MenuSystem::EnqueueMessage(const std::string& message, const std::function<void()>& onComplete) {
    textbox.EnqueueMessage(message, onComplete);
  }

  void MenuSystem::EnqueueQuestion(const std::string& prompt, const std::function<void(bool)>& onResponse) {
    textbox.EnqueueQuestion(prompt, onResponse);
  }

  void MenuSystem::EnqueueQuiz(const std::string& optionA, const std::string& optionB, const std::string& optionC, const std::function<void(int)>& onResponse) {
    textbox.EnqueueQuiz(optionA, optionB, optionC, onResponse);
  }

  void MenuSystem::EnqueueTextInput(const std::string& initialText, size_t characterLimit, const std::function<void(const std::string&)>& onResponse) {
    textbox.EnqueueTextInput(initialText, characterLimit, onResponse);
  }

  bool MenuSystem::IsOpen() {
    return activeBindedMenu || textbox.IsOpen() || bbs;
  }

  bool MenuSystem::IsClosed() {
    return !activeBindedMenu && textbox.IsClosed() && !bbs;
  }

  bool MenuSystem::IsFullscreen() {
    float bbsAnimationProgress = bbsFadeDuration / BBS_FADE_DURATION_MAX;
    return (activeBindedMenu && activeBindedMenu->IsFullscreen()) || (bbs && bbsAnimationProgress > 0.5f);
  }

  bool MenuSystem::ShouldLockInput() {
    return (activeBindedMenu && activeBindedMenu->LocksInput()) || textbox.IsOpen() || bbs;
  }

  void MenuSystem::Update(float elapsed) {
    for (auto& [binding, menu] : bindedMenus) {
      menu->Update(elapsed);
    }

    if (bbsFadeDuration < BBS_FADE_DURATION_MAX) {
      bbsFadeDuration += elapsed;
    }

    float bbsAnimationProgress = bbsFadeDuration / BBS_FADE_DURATION_MAX;

    if (bbsAnimationProgress >= 0.5) {
      if (bbs) {
        bbs->Update(elapsed);
      }

      closingBbs = nullptr;
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

    if (bbs) {
      if (!bbsNeedsAck) {
        bbs->HandleInput(input);
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
    float bbsAnimationProgress = bbsFadeDuration / BBS_FADE_DURATION_MAX;

    if (bbs && bbsAnimationProgress >= 0.5) {
      target.draw(*bbs, states);
    }

    if (closingBbs) {
      target.draw(*closingBbs, states);
    }

    textbox.draw(target, states);

    if (bbsAnimationProgress < 1.0) {
      float alpha = swoosh::ease::wideParabola(bbsFadeDuration, BBS_FADE_DURATION_MAX, 1.0f);

      sf::RectangleShape fade;
      fade.setSize(sf::Vector2f(480, 320));
      fade.setFillColor(sf::Color(0, 0, 0, sf::Uint8(alpha * 255)));
      target.draw(fade, states);
    }

    if (activeBindedMenu) {
      activeBindedMenu->draw(target, states);
    }
  }
}
