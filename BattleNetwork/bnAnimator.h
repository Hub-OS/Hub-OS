#pragma once

#include <SFML/Graphics.hpp>
#include <map>
#include <functional>
#include <assert.h>
#include <iostream>
#include <list>

#include "bnLogger.h"

struct OverrideFrame {
  int frameIndex;
  double duration;
};

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

  Frame(float duration, sf::IntRect subregion, bool applyOrigin, sf::Vector2f origin) 
  : duration(duration), subregion(subregion), applyOrigin(applyOrigin), origin(origin) {
    points["ORIGIN"] = origin;
  }

  Frame(const Frame& rhs) {
    *this = rhs;
  }

  Frame& operator=(const Frame& rhs) {
    duration = rhs.duration;
    subregion = rhs.subregion;
    applyOrigin = rhs.applyOrigin;
    origin = rhs.origin;
    points = rhs.points;

    return *this;
  }


  Frame& operator=(Frame&& rhs) {
    duration = rhs.duration;
    rhs.duration = 0;

    subregion = rhs.subregion;

    applyOrigin = rhs.applyOrigin;
    rhs.applyOrigin = false;

    origin = rhs.origin;
    points = rhs.points;
    rhs.points.clear();
    return *this;
  }

  Frame(Frame&& rhs) {
    *this = rhs;
  }
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
  friend class Animator;

  FrameList() { totalDuration = 0; }
  FrameList(const FrameList& rhs) { 
    frames = rhs.frames; 
    totalDuration = rhs.totalDuration;
  }

  FrameList MakeNewFromOverrideData(std::list<OverrideFrame> data) {
    auto iter = data.begin();

    FrameList res;

    while (iter != data.end() && data.size() > 0) {
      auto copy = this->frames[iter->frameIndex - 1];
      copy.duration = (float)iter->duration;
      res.frames.push_back(copy);
      res.totalDuration += copy.duration;
      iter++;
    }

    return res;
  }

  /**
   * @brief Adds frame to list with an origin at (0,0) and increases totalDuration
   * @param dur duration of frame in seconds
   * @param sub int rectangle defining the frame from a texture sheet
   */
  void Add(float dur, sf::IntRect sub) {
    frames.emplace_back(std::move(Frame(dur, sub, false, sf::Vector2f(0,0) )));
    totalDuration += dur;
  }

  /**
   * @brief Adds frame to the list with a defined origin and increases totalDuration
   * @param dur duration of frame in seconds
   * @param sub int rectangle defininf the frame from a texture sheet
   * @param origin origin of frame
   */
  void Add(float dur, sf::IntRect sub, sf::Vector2f origin) {
    frames.emplace_back(std::move(Frame(dur, sub, true, origin)));
    totalDuration += dur;
  }

  /**
  * @brief sets the (x,y) location for a point and assigns a name
  * @param name of the new point 
  * @param x of the vector
  * @param y of the vector
  * Will overwrite any other point with the same name in the frame - unique names only
  */
  void SetPoint(const std::string& name, int x, int y) {
    auto str = name;
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    frames[frames.size() - 1].points[name] = sf::Vector2f(float(x), float(y));
  }

  /**
 * @brief Get the total number of frames in this list
 * @return const unsigned int
 */
  const size_t GetFrameCount() { return this->frames.size(); }

  /**
  * @brief Get the frame data at the given index
  * @param index of the frame in the list (base 0)
  * @return const Frame immutable
  */
  const Frame& GetFrame(const int index) { return this->frames[index]; }

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
 * @class Animator
 * @author mav
 * @date 13/05/19
 * @brief Robust animator class that handles callbacks of many kinds
 * 
 * Animator updates a frame list independantly and can be used on multiple frame lists
 * 
 * It can set callbacks on frame numbers, one time callbacks, and will intelligently
 * queue callbacks if callbacks are being added during a callback itself.
 */
class Animator {
private:
  std::multimap<int, std::function<void()>> callbacks; /*!< Called every time on frame */
  std::multimap<int, std::function<void()>> onetimeCallbacks; /*!< Called once on frame then discarded */
  std::multimap<int, std::function<void()>> nextLoopCallbacks; /*!< used to queue already called callbacks */
  std::multimap<int, std::function<void()>> queuedCallbacks; /*!< used for adding new callbacks while updating */
  std::multimap<int, std::function<void()>> queuedOnetimeCallbacks; /*!< adding new one-time callbacks in update */
  
  std::map<std::string, sf::Vector2f> currentPoints;
  
  std::function<void()> onFinish; /*!< special callback that fires when the animation is completed */
  std::function<void()> queuedOnFinish; /*!< Queues onFinish callback when used in the middle of update */
  
  char playbackMode; /*!< determins how to animate the frame list */
  
  bool isUpdating; /*!< Flag if in the middle of update */
  bool callbacksAreValid; /*!< Flag for queues. If false, all added callbacks are discarded. */
  
  void UpdateCurrentPoints(int frameIndex, FrameList& sequence);

public:
  inline static const std::function<void()> NoCallback = [](){};

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

    friend class Animator;
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
    
    friend class Animator;
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

  Animator();
  Animator(const Animator& rhs);
  Animator& operator=(const Animator& rhs);
  ~Animator();

  /**
   * @brief Get the current playback mode
   * @return char
   */
  char GetMode() { return playbackMode;  }
  
  const sf::Vector2f GetPoint(const std::string& pointName);
  
  /**
   * @brief Clears all callback functors
   */
  void Clear();

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
   * @return Animator& for chaining 
   */
  Animator& operator<< (On rhs);
  
  /**
   * @brief Applies a playback mode
   * @param rhs byte mode
   * @return Animator& for chaining
   */
  Animator& operator<< (char rhs);
  
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
