#pragma once

#include "bnOverworldActor.h"
#include "../bnCallback.h"
#include "../bnResourceHandle.h"

namespace Overworld {
  enum class ActorProperty : uint8_t {
    animation,
    x,
    y,
    z,
    scale_x,
    scale_y,
    rotation,
    direction,
    sound_effect,
    sound_effect_loop,
  };

  // based on https://developer.mozilla.org/en-US/docs/Web/CSS/easing-function
  enum class Ease : uint8_t {
    linear,
    in,
    out,
    in_out,
    floor // similar to steps
  };

  class ActorPropertyAnimator : public ResourceHandle {
  public:
    struct PropertyStep {
      ActorProperty property;
      Ease ease;
      float value;
      std::string stringValue;
    };

    struct KeyFrame {
      std::vector<PropertyStep> propertySteps;
      double duration{};
    };

    bool IsAnimating();
    bool IsAnimatingPosition();

    void ToggleAudio(bool);

    void Reset();
    void AddKeyFrame(KeyFrame keyframe);
    void UseKeyFrames(Actor& actor);
    void Update(Actor& actor, double elapsed);

    void OnComplete(const std::function<void()> callback) { onComplete = callback; }
  private:
    struct PropertyKeyFrame {
      Ease ease;
      float previousValue;
      float value;
      std::string previousStringValue;
      std::string stringValue;
      double previousTimePoint{};
      double timePoint{};
    };

    struct PropertyState {
      bool done{};
      size_t currentKeyFrame{};
      std::vector<PropertyKeyFrame> keyFrames;
    };

    std::unordered_map<ActorProperty, PropertyState> propertyStates;
    double time{};
    double totalTime{};
    bool animating{};
    bool animatingPosition{};
    bool animatingDirection{};
    bool audioEnabled = true;
    std::function<void()> onComplete;
  };
}
