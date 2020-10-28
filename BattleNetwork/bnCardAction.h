#pragma once
#include <Swoosh/ActionList.h>
#include <SFML/Graphics.hpp>
#include <string>
#include <functional>
#include <map>

#include "bnUIComponent.h"
#include "bnAnimationComponent.h"
#include "bnCharacter.h"
#include "bnSpriteProxyNode.h"

using namespace swoosh;

enum class ActionLockoutType : unsigned {
  animation = 0,
  async,
  sequence
};

enum class ActionLockoutGroup : unsigned {
  none = 0,
  card,
  ability
};

struct ActionLockoutProperties {
  ActionLockoutType type{};
  double cooldown{};
  ActionLockoutGroup group{};
};

class CardAction : public Component, public SceneNode {
public:
  struct Attachment {
    std::reference_wrapper<SpriteProxyNode> spriteProxy;
    std::reference_wrapper<Animation> parentAnim;
    Animation myAnim;

    void PrepareAnimation(const Animation&);
    void Update(double elapsed);
    void SetOffset(const sf::Vector2f& pos);
    Animation& GetParentAnim();
  };

  struct Step : public swoosh::ActionItem {
    std::function<void(double)> updateFunc;
    std::function<void(sf::RenderTexture&)> drawFunc;

    // inherited functions simply invoke the functors
    void update(double elapsed) override {
      if (updateFunc) updateFunc(elapsed);
    }
    void draw(sf::RenderTexture& surface) override {
      if (drawFunc) drawFunc(surface);
    }
  };

  using Attachments = std::multimap<std::string, Attachment>;

private:
  bool animationIsOver{ false };
  ActionLockoutProperties lockoutProps{};
  std::string animation;
  std::string uuid, prevState;
  std::function<void()> prepareActionDelegate;
  ActionList sequence;
  Character& user;
  Attachments attachments;
  AnimationComponent* anim{ nullptr };

  // Used internally
  void RecallPreviousState();
  void FreeAttachedNodes();

protected:
  /*user defined */
  virtual void Execute() = 0;

  // For sequences
  void AddStep(Step* step);

  // For simple animations
  void AddAnimAction(int frame, const FrameCallback& action);

  // For additional visual overlays
  Attachment& AddAttachment(Animation& parent, const std::string& point, SpriteProxyNode& node);

  // Shortcut to add an attachment to a character via their animation component
  Attachment& AddAttachment(Character& character, const std::string& point, SpriteProxyNode& node);

  // Calculate the offset for an attachment for a given point in the owner's animation set
  sf::Vector2f CalculatePointOffset(const std::string& point);

public:
  CardAction() = delete;
  CardAction(const CardAction& rhs) = delete;
  CardAction(Character& owner, const std::string& animation = "PLAYER_IDLE");
  virtual ~CardAction();

  virtual void OnUpdate(float _elapsed);
  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
  virtual void EndAction() = 0;
  virtual void OnAnimationEnd() = 0;

  void Inject(BattleSceneBase&) final;
  void SetLockout(const ActionLockoutProperties& props);
  void SetLockoutGroup(const ActionLockoutGroup& group);
  void OverrideAnimationFrames(std::list<OverrideFrame> frameData);
  void OnExecute();

  const ActionLockoutGroup GetLockoutGroup() const;
  const ActionLockoutType GetLockoutType() const;
  const bool IsAnimationOver() const;
  const bool IsLockoutOver() const;
  /** Override get owner to always return a character type */
  Character* GetOwner();
};