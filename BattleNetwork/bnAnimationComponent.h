#pragma once
#include <string>
#include <assert.h>
#include <functional>

#include "bnComponent.h"
#include "bnAnimation.h"

using std::string;
using std::to_string;

class Entity;

/**
 * @class AnimationComponent
 * @author mav
 * @date 14/05/19
 *
 * A component to add to entities to provide animations.
 *
 */
class AnimationComponent : public Component {
public:
  /**
   @param _entity Owner of this component
    */
  AnimationComponent(Entity* _entity);
  virtual ~AnimationComponent();

  /**
   * @brief Delegates work to animation object
   * @param _elapsed in seconds
   */
  virtual void OnUpdate(float _elapsed);

  /**
   * @brief Does not inject into scene. Used by the owner.
   * @param BattleScene& unused
   */
  virtual void Inject(BattleScene&) { ; }
  
  /**
   * @brief Reconstructs the animation object
   * @param _path to animation file
   */
  void Setup(string _path);

  /**
   * @brief Loads the animation object
   */
  void Load();

  /**
   * @brief Reload the animation object. Same as Load()
   */
  void Reload();
  
  /**
   * @brief Get animation object's current animation name
   * @return name of animation or empty string if no state
   */
  const std::string GetAnimationString() const;
  
  /**
   * @brief Set the animation playback speed
   * @param playbackSpeed factor of 100 e.g. 2 is 200%, 0.5 is 50%
   */
  void SetPlaybackSpeed(const double playbackSpeed);
  
  /**
   * @brief Set the playback mode
   * @param playbackMode byte mode
   */
  void SetPlaybackMode(char playbackMode);
  
  /**
   * @brief Set the animation and provide an on finish notifier
   * @param state
   */
  void SetAnimation(string state, std::function<void()> onFinish = nullptr);
  
  /**
   * @brief Set the animation, set the playback mode, and provide an on finish notifier
   * @param state
   * @param playbackMode
   */
  void SetAnimation(string state, char playbackMode, std::function<void()> onFinish = std::function<void()>());
  
  /**
   * @brief Add a frame callback
   * @param frame Base 1 index
   * @param onFrame Callback when frame is entered
   * @param outFrame Callback when frame is left
   * @param doOnce Toggle if the callback should be fired every time the animation loops or just once
   */
  void AddCallback(int frame, std::function<void()> onFrame, std::function<void()> outFrame = std::function<void()>(), bool doOnce = false);
  
  /**
   * @brief Clears callbacks
   */
  void CancelCallbacks();
  
private:
  string path; /*!< Path to animation */
  Animation animation; /*!< Animation object */
  double speed; /*!< Playback speed */
};
