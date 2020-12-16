#pragma once
#include <Swoosh/ActionList.h>
#include <SFML/Graphics.hpp>
#include <string>
#include <functional>
#include <map>

#include "bnUIComponent.h"
#include "bnCharacter.h"
#include "bnSpriteProxyNode.h"
#include "bnResourceHandle.h"
#include "bnAnimationComponent.h"
#include "bnCharacter.h"


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
  double cooldown{}; // in seconds
  ActionLockoutGroup group{ ActionLockoutGroup::card };
};

class CardAction : public Component, public SceneNode, public ResourceHandle {
public:
  class Attachment {
    using Attachments = std::multimap<std::string, Attachment>;

    bool started{ false };
    std::reference_wrapper<SpriteProxyNode> spriteProxy;
    std::reference_wrapper<Animation> parentAnim;
    Attachments attachments;
    Animation* myAnim{ nullptr };

  public:
    friend class CardAction; // Let CardAction inspect our member vars

    Attachment(SpriteProxyNode& parentNode, Animation& parentAnim);
    ~Attachment();

    Attachment& UseAnimation(Animation&);
    void Update(double elapsed);
    void SetOffset(const sf::Vector2f& pos);
    void AttachAllPendingNodes();
    Animation& GetParentAnim();
    Attachment& AddAttachment(Animation& parent, const std::string& point, SpriteProxyNode& node);
  };

  struct Step : public swoosh::BlockingActionItem {
    using UpdateFunc_t = std::function<void(double, Step&)>;
    using DrawFunc_t = std::function<void(sf::RenderTexture&, Step&)>;
    
    constexpr static auto NoUpdateFunc = [](double, Step&) -> void {};
    constexpr static auto NoDrawFunc = [](sf::RenderTexture&, Step&) -> void {};

    UpdateFunc_t updateFunc;
    DrawFunc_t drawFunc;

    Step(const UpdateFunc_t& u = NoUpdateFunc, const DrawFunc_t& d = NoDrawFunc) :
      updateFunc(u), drawFunc(d) {}

    // inherited functions simply invoke the functors
    void update(double elapsed) override {
      if (updateFunc) updateFunc(elapsed, *this);
    }
    void draw(sf::RenderTexture& surface) override {
      if (drawFunc) drawFunc(surface, *this);
    }
  };

  using Attachments = std::multimap<std::string, Attachment>;

private:
  bool animationIsOver{ false };
  bool started{ false };
  ActionLockoutProperties lockoutProps{};
  std::string animation;
  std::string uuid, prevState;
  std::function<void()> prepareActionDelegate;
  ActionList sequence;
  std::list<Step*> stepList; //!< Swooshlib needs pointers so we must copy steps and put them on the heap
  Character& user;
  Attachments attachments;
  AnimationComponent* anim{ nullptr };

  // Used internally
  void RecallPreviousState();
  void FreeAttachedNodes();

protected:
  // Used by cards that use sequences (like most Time Freeze animations)
  void AddStep(Step step);

  // Used with basic cards that perform some action
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

  void Inject(BattleSceneBase&) final;
  void SetLockout(const ActionLockoutProperties& props);
  void SetLockoutGroup(const ActionLockoutGroup& group);
  void OverrideAnimationFrames(std::list<OverrideFrame> frameData);
  void Execute();
  void EndAction();

  const ActionLockoutGroup GetLockoutGroup() const;
  const ActionLockoutType GetLockoutType() const;
  const bool IsAnimationOver() const;
  const bool IsLockoutOver() const;
  const bool CanExecute() const;

  /** Override get owner to always return a character type */
  Character* GetOwner();

  virtual void OnUpdate(double _elapsed);
  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;

protected:
  virtual void OnEndAction() = 0;
  virtual void OnAnimationEnd() = 0;
  virtual void OnExecute() = 0;
};