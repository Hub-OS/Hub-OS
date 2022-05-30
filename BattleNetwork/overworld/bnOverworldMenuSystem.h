#pragma once

#include "../bnResourceHandle.h"
#include "../bnInputManager.h"
#include "../bnMenu.h"
#include "bnOverworldTextBox.h"
#include "bnBBS.h"
#include <SFML/Graphics/Drawable.hpp>
#include <functional>
#include <optional>

namespace Overworld {
  class MenuSystem : public sf::Drawable, ResourceHandle {
  public:
    MenuSystem();

    /**
     * @brief Binds an input event to open a menu, avoids opening menus while other menus are open
     */
    void BindMenu(InputEvent event, std::shared_ptr<Menu> menu);

    // grabs the latest BBS
    std::optional<std::reference_wrapper<BBS>> GetBBS();
    void OpenBBS(const std::string& topic, sf::Color color, bool openInstantly, const std::function<void(const std::string&)>& onSelect, const std::function<void()>& onClose);
    void CloseBBS();
    void AcknowledgeBBSSelection();

    void SetNextSpeaker(const sf::Sprite& speaker, const Animation& animation);
    void EnqueueMessage(const std::string& message, const std::function<void()>& onComplete = []() {});
    void EnqueueQuestion(const std::string& prompt, const std::function<void(bool)>& onResponse);
    void EnqueueQuiz(
      const std::string& optionA,
      const std::string& optionB,
      const std::string& optionC,
      const std::function<void(int)>& onResponse
    );
    void EnqueueTextInput(const std::string& initialText, size_t characterLimit, const std::function<void(const std::string&)>& onResponse);

    bool IsOpen();
    bool IsClosed();
    bool IsFullscreen();
    bool ShouldLockInput();

    void Update(float elapsed);
    void HandleInput(InputManager& input, sf::Vector2f mousePos);

    void draw(sf::RenderTarget& target, sf::RenderStates states) const;

  private:
    std::vector<std::pair<InputEvent, std::shared_ptr<Menu>>> bindedMenus;
    std::shared_ptr<Menu> activeBindedMenu;
    std::unique_ptr<BBS> bbs;
    std::unique_ptr<BBS> closingBbs;
    bool bbsOpening{ };
    float bbsFadeDuration{ 1.0f };
    Overworld::TextArea textbox;
    bool bbsNeedsAck{ };
  };
}
