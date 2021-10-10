#pragma once
#include <Swoosh/ActionList.h>
#include <SFML/Graphics.hpp>
#include <string>
#include <functional>
#include <map>
#include "stx/memory.h"

#include "bnSpriteProxyNode.h"
#include "bnResourceHandle.h"
#include "bnAnimationComponent.h"
#include "bnCard.h"

class Character;

using namespace swoosh;

class CardAction : public stx::enable_shared_from_base<CardAction>, public sf::Drawable, public ResourceHandle {
public:
  enum class LockoutType : unsigned {
    animation = 0,
    async,
    sequence
  };

  enum class LockoutGroup : unsigned {
    none = 0,
    weapon,
    card,
    ability
  };

  struct LockoutProperties {
    LockoutType type{};
    double cooldown{}; // in seconds
    LockoutGroup group{ LockoutGroup::card };
  };

  class Attachment {
    using Attachments = std::multimap<std::string, Attachment>;

    bool started{ false };
    std::string point;
    std::reference_wrapper<SpriteProxyNode> spriteProxy;
    std::reference_wrapper<Animation> parentAnim;
    Attachments attachments;
    Animation* myAnim{ nullptr };

  public:
    friend class CardAction; // Let CardAction inspect our member vars

    Attachment(Animation& parentAnim, const std::string& point, SpriteProxyNode& parentNode);
    ~Attachment();

    Attachment& UseAnimation(Animation&);
    void Update(double elapsed);
    void SetOffset(const sf::Vector2f& pos);
    void SetScale(const sf::Vector2f& scale);
    void AttachAllPendingNodes();
    Animation& GetParentAnim();
    Attachment& AddAttachment(Animation& parent, const std::string& point, SpriteProxyNode& node);
  };

  struct Step : public swoosh::BlockingActionItem {
    using UpdateFunc_t = std::function<void(Step&, double)>;
    using DrawFunc_t = std::function<void(Step&, sf::RenderTexture&)>;
    
    constexpr static auto NoUpdateFunc = [](Step&, double) -> void {};
    constexpr static auto NoDrawFunc = [](Step&, sf::RenderTexture&) -> void {};

    UpdateFunc_t updateFunc;
    DrawFunc_t drawFunc;

    Step(const UpdateFunc_t& u = NoUpdateFunc, const DrawFunc_t& d = NoDrawFunc) :
      updateFunc(u), drawFunc(d) {}

    // inherited functions simply invoke the functors
    void update(sf::Time elapsed) override {
      if (updateFunc) updateFunc(*this, elapsed.asSeconds());
    }
    void draw(sf::RenderTexture& surface) override {
      if (drawFunc) drawFunc(*this, surface);
    }
  };

  using Attachments = std::multimap<std::string, Attachment>;

private:
  bool animationIsOver{ false };
  bool started{ false };
  bool recalledAnimation{ false };
  bool preventCounters{ false };
  LockoutProperties lockoutProps{};
  std::string animation;
  std::string uuid, prevState;
  std::function<void()> prepareActionDelegate;
  ActionList sequence;
  std::list<Step*> stepList; //!< Swooshlib needs pointers so we must copy steps and put them on the heap
  std::weak_ptr<Character> actor;
  Attachments attachments;
  std::shared_ptr<AnimationComponent> anim{ nullptr };
  Battle::Card::Properties meta;

  // Used internally
  void RecallPreviousState();
  void FreeAttachedNodes();

public:
  CardAction() = delete;
  CardAction(const CardAction& rhs) = delete;
  CardAction(std::weak_ptr<Character> actor, const std::string& animation);
  virtual ~CardAction();

  // Used by cards that use sequences (like most Time Freeze animations)
  void AddStep(Step step);

  // Used with basic cards that perform some action
  void AddAnimAction(int frame, const FrameCallback& action);

  // For additional visual overlays
  Attachment& AddAttachment(Animation& parent, const std::string& point, SpriteProxyNode& node);

  // Shortcut to add an attachment to a character via their animation component
  Attachment& AddAttachment(std::shared_ptr<Character> character, const std::string& point, SpriteProxyNode& node);

  // Calculate the offset for an attachment for a given point in the owner's animation set
  sf::Vector2f CalculatePointOffset(const std::string& point);

  void PreventCounters();
  void SetLockout(const LockoutProperties& props);
  void SetLockoutGroup(const LockoutGroup& group);
  void OverrideAnimationFrames(std::list<OverrideFrame> frameData);
  void SetMetaData(const Battle::Card::Properties& props);
  void Execute(std::shared_ptr<Character> user);
  void EndAction();
  void UseStuntDouble(std::shared_ptr<Character> stuntDouble); // can cause GetActor to return nullptr

  const LockoutGroup GetLockoutGroup() const;
  const LockoutType GetLockoutType() const;
  const std::string& GetAnimState() const;
  const bool IsAnimationOver() const;
  const bool IsLockoutOver() const;
  const Battle::Card::Properties& GetMetaData() const;
  const bool CanExecute() const;
  std::shared_ptr<Character> GetActor(); // may return nullptr
  const std::shared_ptr<Character> GetActor() const; // may return nullptr

  virtual void Update(double _elapsed);
  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;

protected:
  virtual void OnActionEnd() = 0;
  virtual void OnAnimationEnd() = 0;
  virtual void OnExecute(std::shared_ptr<Character> user) = 0;
};
