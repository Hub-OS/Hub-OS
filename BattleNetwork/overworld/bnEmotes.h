#pragma once
#include <Swoosh/Timer.h>
#include "../bnInputHandle.h"
#include "../bnResourceHandle.h"
#include "../bnSpriteProxyNode.h"
#include "../bnCallback.h"

namespace Overworld {
  enum class Emotes : size_t {
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
    size
  };

  class EmoteNode : public SpriteProxyNode, public ResourceHandle {
    Emotes currEmote{};
    swoosh::Timer timer{};
  public:

    EmoteNode();
    ~EmoteNode();

    void Reset();
    void Emote(Emotes type);
    void Update(double elapsed);
  };

  class EmoteWidget : 
    public sf::Drawable, 
    public sf::Transformable, 
    public ResourceHandle, 
    public InputHandle {
    float radius{ 25.0f }; //!< in pixels
    Emotes currEmote{};
    SpriteProxyNode emoteSprites[static_cast<size_t>(Emotes::size)];
    std::function<void(Emotes)> callback;

    enum class State : unsigned {
      closed = 0,
      open
    } state{State::closed};
  public:
    EmoteWidget();
    ~EmoteWidget();
    void Update(double elapsed);
    void draw(sf::RenderTarget& surface, sf::RenderStates states) const override;
    void Open();
    void Close();
    void OnSelect(const std::function<void(Emotes)>& callback);
    const bool IsOpen();
    const bool IsClosed();
  };
}