#pragma once

#include "../bnResourceHandle.h"
#include "../bnInputManager.h"
#include "../bnMessageInterface.h"
#include "../bnAnimatedTextBox.h"

#include <SFML/Graphics/Drawable.hpp>
#include <functional>

namespace Overworld {
  class AnimatedTextBox : public sf::Drawable, ResourceHandle {
  public:
    AnimatedTextBox(sf::Vector2f pos);

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
    size_t GetRemainingMessages();

    void Update(float elapsed);
    void HandleInput(InputManager& input, sf::Vector2f mousePos);
    void ChangeAppearance(std::shared_ptr<sf::Texture> newTexture, const Animation& newAnimation);
    void ChangeBlipSfx(std::shared_ptr<sf::SoundBuffer> newSfx);
    void draw(sf::RenderTarget& target, sf::RenderStates states) const;

  private:
    ::AnimatedTextBox textbox;
    sf::Sprite nextSpeaker;
    Animation nextAnimation;
    std::queue<std::function<void(InputManager& input, sf::Vector2f mousePos)>> handlerQueue;
    bool turboScroll;
    int turboTimer;
  };
}
