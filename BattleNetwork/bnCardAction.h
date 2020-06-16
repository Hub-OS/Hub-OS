#pragma once
#include "bnResourceHandle.h"
#include "bnAnimationComponent.h"
#include "bnCharacter.h"
#include "bnSpriteProxyNode.h"

#include <string>
#include <functional>

#include <SFML/Graphics.hpp>

// TODO: Refactor this further when using transition system: SimpleCardAction (this) and CardAction
//       SimpleCardAction will take in an animation like this class currently does
//       while CardAction will use Swoosh's action lists to create sequences
//       this will allow card actions to be used for TFC and actions that use the transition system

/**
* @brief This class defines a framework for creating custom attack behaviors from simple to complex sequences 
*
* Cards effectively freeze a Character from recieving any more input and will only return to their previous
* animation (mechanically, always default/idle state animation) when the sequence is complete or they were interrupted.
* Most card actions add overlays as attachment scene nodes onto the animation's target point.
* These overlays can also be animated.
* The character's animation can be overriden to reuse frames from existing animations for complex behavior.
*/
class CardAction : public ResourceHandle {
public:
  struct NodeAttachment {
    std::reference_wrapper<SpriteProxyNode> spriteProxy;
    std::reference_wrapper<Animation> parentAnim;
    Animation anim;

    void PrepareAnimation(const Animation& animation) {
      anim = animation;
    }

    void Update(float elapsed) {
      anim.Update(elapsed, spriteProxy.get().getSprite());
    }

    void SetPosition(const sf::Vector2f& pos) {
      spriteProxy.get().setPosition(pos);
    }

    Animation& GetParent() {
      // By design of the node attachment system, we will ALWAYS have a parent
      return parentAnim;
    }

    NodeAttachment() = default;
    ~NodeAttachment() = default;
  };

  using Attachments = std::multimap<std::string,NodeAttachment>;
protected:
  Character& user;
  AnimationComponent* anim;
  std::string animation;
  std::string uuid, prevState;
  Attachments attachments;
  std::function<void()> prepareActionDelegate;
  std::list<Entity*> tracked;

protected:
  /*user defined */
  void Execute() {
    // prepare the animation behavior
    prepareActionDelegate();

    for (auto&&[nodeName, node] : attachments) {
      GetUser().AddNode(&node.spriteProxy.get());
    }

    // run
    OnExecute();
  }

  void TrackEntity(Entity& entity) {
    tracked.insert(tracked.begin(), &entity);
  }

  void AddAction(int frame, std::function<void()> action)
  {
    anim->AddCallback(frame, action, true);
  }

  NodeAttachment& AddAttachment(Animation& parent, const std::string& point, SpriteProxyNode& node) {
    auto iter = attachments.insert(std::make_pair(point, NodeAttachment{ std::ref(node), std::ref(parent) }));
    return iter->second;
  }

  void RecallPreviousState() {
    if (anim) {
      anim->SetAnimation(prevState);
      anim->SetPlaybackMode(Animator::Mode::Loop);
    }
  }

public:
  Character& GetUser() { return user; }

  CardAction() = delete;
  CardAction(const CardAction& rhs) = delete;

  CardAction(Character& user, const std::string& animation) 
   user(user), animation(animation), anim(user.GetFirstComponent<AnimationComponent>(),
   uuid(), prevState(), attachments(), tracked() 
 {
    if (anim) {
      prepareActionDelegate = [this, animation]() {
        prevState = anim->GetAnimationString();

        // use the current animation's arrangement, do not overload
        prevState = anim->GetAnimationString();
        anim->SetAnimation(animation, [this]() {
          anim->SetPlaybackMode(Animator::Mode::Loop);
          RecallPreviousState();
          OnEndAction();
        });

        anim->OnUpdate(0);
        OnUpdate(0); // position attachments to owner...

      };
    }
  }

  void OverrideAnimationFrames(std::list<OverrideFrame> frameData)
  {
    if (anim) {
      prepareActionDelegate = [this, frameData]() {
        prevState = anim->GetAnimationString();
        
        anim->OverrideAnimationFrames(animation, frameData, uuid);
        anim->SetAnimation(uuid, [this]() {
          anim->SetPlaybackMode(Animator::Mode::Loop);
          RecallPreviousState();
          OnEndAction();
        });
        anim->OnUpdate(0);
        OnUpdate(0); // position attachments to owner...
      };
    }
  }

  virtual ~CardAction()
  {
  }

  virtual void OnExecute() = 0;

  virtual void OnUpdate(float _elapsed)
  {
    auto& user = GetUser();

    // update node positions in the animation
    for (auto&&[nodeName, node] : attachments) {
      // update the node's animation
      node.Update(_elapsed);
 
      // update the node's position
      auto baseOffset = node.GetParent().GetPoint(nodeName);
      auto origin = user.getSprite().getOrigin();
      baseOffset = baseOffset - origin;

      node.SetPosition(baseOffset);
    }
  }

  void EndAction() {
    for (auto&& e : tracked) {
      e->Delete();
    }

    OnEndAction();

    // MUST be last b/c we delete this
    GetUser().EndCurrentAction();
  }

  virtual void OnEndAction() = 0;
};