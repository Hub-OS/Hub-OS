#pragma once
#include "bnComponent.h"
#include "bnAnimationComponent.h"
#include "bnCharacter.h"
#include "bnSpriteSceneNode.h"
#include <string>
#include <functional>

#include <SFML/Graphics.hpp>

class ChipAction : public Component {
private:
  AnimationComponent* anim;
  std::string animation, nodeName;
  std::string uuid, prevState;
  SpriteSceneNode** attachment;
protected:
  void AddAction(int frame, std::function<void()> action)
  {
    anim->AddCallback(frame, action, std::function<void()>(), true);
  }


public:
  ChipAction() = delete;
  ChipAction(const ChipAction& rhs) = delete;

  ChipAction(Character * owner, std::string animation, SpriteSceneNode** attachment, std::string nodeName)
    : Component(owner), animation(animation), nodeName(nodeName), attachment(attachment)
  {
    anim = owner->GetFirstComponent<AnimationComponent>();

    if (anim) {
      prevState = anim->GetAnimationString();
    }
  }

  void OverrideAnimationFrames(std::list<OverrideFrame> frameData)
  {
    if (anim) {
      anim->OverrideAnimationFrames(this->animation, std::move(frameData), this->uuid);
      anim->SetAnimation(this->uuid, [this]() { 
        anim->SetAnimation(prevState);
        this->EndAction();
      });
      anim->OnUpdate(0);
      this->OnUpdate(0); // position to owner...
    }
  }

  virtual ~ChipAction()
  {
  }

  virtual void OnUpdate(float _elapsed)
  {
    if (!GetOwner() || !GetOwner()->GetTile() || !attachment) return;

    // update node position in the animation
    auto baseOffset = anim->GetPoint(nodeName);
    auto origin = GetOwner()->operator sf::Sprite &().getOrigin();
    baseOffset = baseOffset - origin;

    (*attachment)->setPosition(baseOffset);
  }

  virtual void EndAction() = 0;

  void Inject(BattleScene &) final
  {
    // do nothing
  }
};