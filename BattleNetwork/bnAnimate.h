#pragma once

#include <SFML/Graphics.hpp>
#include <map>
#include <functional>
#include <assert.h>
#include <iostream>

#include "bnLogger.h"

/**
 * @struct Frame
 * @author mav
 * @date 13/05/19
 * @brief Lightweight frame is just an integer rect, duration (in seconds), an origin, and points
 */
struct Frame {
  float duration;
  sf::IntRect subregion;
  bool applyOrigin;
  sf::Vector2f origin;
  
  std::map<std::string, sf::Vector2f> points;
};

/**
 * @class FrameList
 * @author mav
 * @date 13/05/19
 * @brief List of frames
 */
class FrameList {
  std::vector<Frame> frames;
  float totalDuration; /*!< Sum of all frame durations */

public:
  friend class Animate;

  FrameList() { totalDuration = 0; }
  FrameList(const FrameList& rhs) { frames = rhs.frames; totalDuration = rhs.totalDuration; }

  /**
   * @brief Adds frame to list with an origin at (0,0) and increases totalDuration
   * @param dur duration of frame in seconds
   * @param sub int rectangle defining the frame from a texture sheet
   */
  void Add(float dur, sf::IntRect sub) {
    frames.push_back({ dur, sub, false, sf::Vector2f(0,0) });
    totalDuration += dur;
  }

  /**
   * @brief Adds frame to the list with a defined origin and increases totalDuration
   * @param dur duration of frame in seconds
   * @param sub int rectangle defininf the frame from a texture sheet
   * @param origin origin of frame
   */
  void Add(float dur, sf::IntRect sub, sf::Vector2f origin) {
    frames.push_back({ dur, sub, true, origin });
    totalDuration += dur;
  }

  /**
   * @brief Get the total duration for the list of frames
   * @return const float
   */
  const float GetTotalDuration() const { return totalDuration; }

  /**
   * @brief Query if frame list is empty
   * @return true if empty, false otherwise
   */
  const bool IsEmpty() const { return frames.empty(); }
};

/**
 * @class Animate
 * @author mav
 * @date 13/05/19
 * @brief Robust animator class that handles callbacks of many kinds
 * 
 * Animator updates a frame list independantly and can be used on multiple frame lists
 * 
 * It can set callbacks on frame numbers, one time callbacks, and will intelligently
 * queue callbacks if callbacks are being added during a callback itself.
 */
class Animate {
private:
  std::map<int, std::function<void()>> callbacks; /*!< Called every time on frame */
  std::map<int, std::function<void()>> onetimeCallbacks; /*!< Called once on frame then discarded */
  std::map<int, std::function<void()>> nextLoopCallbacks; /*!< used to queue already called callbacks */
  std::map<int, std::function<void()>> queuedCallbacks; /*!< used for adding new callbacks while updating */
  std::map<int, std::function<void()>> queuedOnetimeCallbacks; /*!< adding new one-time callbacks in update */
  
  std::map<std::string, sf::Vector2f> currentPoints;
  
  std::function<void()> onFinish; /*!< special callback that fires when the animation is completed */
  std::function<void()> queuedOnFinish; /*!< Queues onFinish callback when used in the middle of update */
  
  char playbackMode; /*!< determins how to animate the frame list */
  
  bool isUpdating; /*!< Flag if in the middle of update */
  bool callbacksAreValid; /*!< Flag for queues. If false, all added callbacks are discarded. */
  
  void UpdateCurrentPoints(int frameIndex, FrameList& sequence);

public:
  /**
   * @struct On
   * @author mav
   * @date 13/05/19
   * @brief Struct to add new callbacks with. Uses base 1 frame indeces.
   */
  struct On {
    int id; /*!< Base 1 frame index */
    std::function<void()> callback; /*!< Callback to queue */
    bool doOnce; /*!< If true, this is a one-time callback */

    friend class Animate;
    On(int id, std::function<void()> callback, bool doOnce = false) : id(id), callback(callback), doOnce(doOnce) {
      ;
    }
    
    On(const On& rhs) {
		  //std::cout << "in cpy constructor" << std::endl;
		  this->id = rhs.id;
		  this->callback = rhs.callback;
		  this->doOnce = rhs.doOnce;
	  }
  };

  /**
   * @struct Mode
   * @author mav
   * @date 13/05/19
   * @brief Defines playback bytes
   */
  struct Mode {
  private:
    int playback;
    
    friend class Animate;
  public:
    static const char NoEffect = 0x00; /*!< Play once and stop */
    static const char Loop = 0x01; /*!< When it reaches the end, restarts animation and resumes */
    static const char Bounce = 0x02; /*!< When it reaches the end, reverse the animation and resume */
    static const char Reverse = 0x04; /*!< Reverse the animation */

    Mode(int playback) {
      this->playback = playback;
    }

    ~Mode() { ; }
  };

  Animate();
  Animate(Animate& rhs);
  ~Animate();

  /**
   * @brief Get the current playback mode
   * @return char
   */
  char GetMode() { return playbackMode;  }
  
  const sf::Vector2f GetPoint(const std::string label) {
      if(currentPoints.find(label) == currentPoints.end()) {
          Logger::Log("Could not find point in current sequence named " + label);
          return sf::Vector2f();
      }
      return currentPoints[label];
  }
  
  /**
   * @brief Clears all callback functors
   */
  void Clear() {
	  callbacksAreValid = false;
	  queuedCallbacks.clear(); queuedOnetimeCallbacks.clear(); queuedOnFinish = nullptr;
	  nextLoopCallbacks.clear(); callbacks.clear(); onetimeCallbacks.clear(); onFinish = nullptr; playbackMode = 0; 
  }

  /**
   * @brief Overload the () operator to apply proper frame onto a sprite 
   * @param progress in seconds, the elapsed time in the animation
   * @param target sprite to apply frames to
   * @param sequence list of frames
   */
  void operator() (float progress, sf::Sprite& target, FrameList& sequence);
  
  /**
   * @brief Applies a callback
   * @param rhs On struct
   * @return Animate& for chaining 
   */
  Animate& operator<< (On rhs);
  
  /**
   * @brief Applies a playback mode
   * @param rhs byte mode
   * @return Animate& for chaining
   */
  Animate& operator<< (char rhs);
  
  /**
   * @brief Sets an onFinish callback. 
   * @important Does not chain. This must come at the end.
   */
  void operator<< (std::function<void()> finishNotifier);

  /**
   * @brief Manually applies the frame at index
   * @param frameIndex Base 1 index to apply frame of
   * @param target sprite to apply frame to
   * @param sequence frame is pulled from list using index
   */
  void SetFrame(int frameIndex, sf::Sprite& target, FrameList& sequence);
};
