#pragma once

#include "../bnMenu.h"
#include "../bnInputHandle.h"
#include "../bnResourceHandle.h"
#include "../bnSpriteProxyNode.h"
#include "../bnCallback.h"
#include <Swoosh/Timer.h>

namespace Overworld {
  enum class Emotes : uint8_t {
    happy = 0,
    blank,
    mad,
    sad,
    smug,
    shocked,
    cob,
    pvp,
    zzz,
    key,
    question,
    alert,
    heart,
    gg,
    lol,
    yes,
    no,
    vomit,
    cool,
    uncool,
    spirit,
    follow,
    size
  };

  class EmoteNode : public SpriteProxyNode, public ResourceHandle {
    bool usingCustom{};
    Emotes currEmote{};
    swoosh::Timer timer{};
    std::shared_ptr<sf::Texture> defaultEmotes, customEmotes;
  public:

    EmoteNode();
    ~EmoteNode();

    void Reset();
    void Emote(Emotes type);
    void LoadCustomEmotes(const std::shared_ptr<sf::Texture>& spritesheet);
    void CustomEmote(uint8_t idx);
    void Update(double elapsed);
  };

  class EmoteWidget :
    public Menu,
    public ResourceHandle {
    float radius{ 25.0f }; //!< in pixels
    Emotes currEmote{};
    SpriteProxyNode emoteSprites[static_cast<size_t>(Emotes::size)];
    std::function<void(Emotes)> callback;
  public:
    EmoteWidget();
    ~EmoteWidget();
    bool LocksInput() override { return false; }
    void Update(double elapsed) override;
    void HandleInput(InputManager& input, sf::Vector2f mousePos) override;
    void draw(sf::RenderTarget& surface, sf::RenderStates states) const override;
    void OnSelect(const std::function<void(Emotes)>& callback);
  };
}