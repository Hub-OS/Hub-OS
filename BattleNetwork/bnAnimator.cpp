#include "bnAnimator.h"

#include <iostream>

Animator::Animator() {
  onFinish = nullptr;
  queuedOnFinish = nullptr;
  isUpdating = false;
  callbacksAreValid = true;
  playbackMode = 0x00;
}

Animator::Animator(const Animator& rhs) {
  *this = rhs;
}

Animator & Animator::operator=(const Animator & rhs)
{
  this->onFinish = rhs.onFinish;
  this->callbacks = rhs.callbacks;
  this->onetimeCallbacks = rhs.onetimeCallbacks;
  this->nextLoopCallbacks = rhs.nextLoopCallbacks;
  this->queuedCallbacks = rhs.queuedCallbacks;
  this->queuedOnetimeCallbacks = rhs.queuedOnetimeCallbacks;
  this->queuedOnFinish = rhs.queuedOnFinish;
  this->isUpdating = rhs.isUpdating;
  this->callbacksAreValid = rhs.callbacksAreValid;
  this->currentPoints = rhs.currentPoints;
  this->playbackMode = rhs.playbackMode;

  return *this;
}

Animator::~Animator() {
  this->callbacks.clear();
  this->queuedCallbacks.clear();
  this->onetimeCallbacks.clear();
  this->queuedOnetimeCallbacks.clear();
  this->nextLoopCallbacks.clear();
  this->onFinish = nullptr;
  this->queuedOnFinish = nullptr;
}

void Animator::UpdateCurrentPoints(int frameIndex, FrameList& sequence) {
  if (sequence.frames.size() <= frameIndex) return;

  currentPoints = sequence.frames[frameIndex].points;
}

void Animator::operator() (float progress, sf::Sprite& target, FrameList& sequence) {
  float startProgress = progress;

  UpdateCurrentPoints(0, sequence);

  // Callbacks are only invalid during clears in the update loop
  if (!callbacksAreValid) callbacksAreValid = true;

  // Set our flag to let callback additions go to the right queue  
  isUpdating = true;

  if (sequence.frames.empty()) {
    if (onFinish != nullptr) {
      // Fire the onFinish callback if available
      onFinish();
      onFinish = nullptr;
    }

    // We've ended the update loop
    isUpdating = false;

    // All callbacks are in a valid state
    callbacksAreValid = true;

    // Insert any queued callbacks into the callback list
    callbacks.insert(queuedCallbacks.begin(), queuedCallbacks.end());
    queuedCallbacks.clear();

    // Insert any queued one-time callbacks into the one-time callback list
    onetimeCallbacks.insert(queuedOnetimeCallbacks.begin(), queuedOnetimeCallbacks.end());
    queuedOnetimeCallbacks.clear();

    // Insert any queued onFinish callback into the onFinish callback
    if (queuedOnFinish) {
      onFinish = queuedOnFinish;
      queuedOnFinish = nullptr;
    }

    // End
    return;
  }

  // Determine if the progress has completed the animation
  bool applyCallback = (sequence.totalDuration == 0 || (startProgress > sequence.totalDuration && startProgress > 0.f));

  if (sequence.totalDuration == 1.2f) {
    Logger::Logf("metal man punch progress: %f", startProgress);

    if (applyCallback) {
      Logger::Logf("Onfinish Notifier is %d", onFinish ? 1 : 0);
      Logger::Logf("Queued Onfinish Notifier is %d", queuedOnFinish ? 1 : 0);

    }
  }

  if (applyCallback) {
    if (onFinish != nullptr) {
      // If applicable, fire the onFinish callback
      onFinish();

      // If we do not loop the animation, empty the onFinish notifier. If we don't empty the notifier, this fires infinitely...
      if ((playbackMode & Mode::Loop) != Mode::Loop) {
        onFinish = nullptr;
      }
    }
    // We've ended the loop
    isUpdating = false;

    // Callbacks are valid
    callbacksAreValid = true;

    // Insert any queued callbacks
    callbacks.insert(queuedCallbacks.begin(), queuedCallbacks.end());
    queuedCallbacks.clear();

    // Insert any queued one-time callbacks
    onetimeCallbacks.insert(queuedOnetimeCallbacks.begin(), queuedOnetimeCallbacks.end());
    queuedOnetimeCallbacks.clear();

    // If any queued onFinish notifiers...
    if (queuedOnFinish) {
      // Add it
      onFinish = queuedOnFinish;
      queuedOnFinish = nullptr;
    }

    // End
    return;
  }

  // We may modify the frame order, make a copy
  std::vector<Frame> copy = sequence.frames;

  // If the playback mode is reverse, flip the frames
  if ((playbackMode & Mode::Reverse) == Mode::Reverse) {
    reverse(copy.begin(), copy.end());
  }

  // frame index
  int index = 0;

  // Iterator to the frame itself
  std::vector<Frame>::const_iterator iter = copy.begin();

  // While there is time left in the progress loop
  while (iter != copy.end() && startProgress != 0.f) {
    // Increase the index
    index++;

    // Subtract from the progress
    progress -= (*iter).duration;

    // Must be <= and not <, to handle case (progress == frame.duration) correctly
    // We assume progress hits zero because we use it as a decrementing counter
    // We add a check to ensure the start progress wasn't also 0
    // If it did not start at zero, we know we came across the end of the animation
    if ((progress <= 0.f || &(*iter) == &copy.back()) && startProgress != 0.f) {
      std::map<int, std::function<void()>>::iterator callbackIter, callbackFind = this->callbacks.find(index);
      std::map<int, std::function<void()>>::iterator onetimeCallbackIter = this->onetimeCallbacks.find(index);

      callbackIter = callbacks.begin();

      while (callbacksAreValid && callbackIter != callbackFind && callbackFind != this->callbacks.end()) {
        if (callbackIter->second) {
          callbackIter->second();
        }

        // If the callback modified the first callbacks list, break
        if (!callbacksAreValid) break;

        // Otherwise add the callback into the next loop queue
        nextLoopCallbacks.insert(*callbackIter);

        // Erase the callback so we don't fire again
        callbackIter = callbacks.erase(callbackIter);

        // Find the callback at the given index
        callbackFind = callbacks.find(index);
      }

        // If callbacks are ok and the iterator matches the expected position
        if (callbacksAreValid && callbackIter == callbackFind && callbackFind != this->callbacks.end()) {
          if (callbackIter->second) {
            callbackIter->second();
          }

          if (callbacksAreValid) {
            nextLoopCallbacks.insert(*callbackIter);
            callbackIter = callbacks.erase(callbackIter);
          }
        }

        if (callbacksAreValid && onetimeCallbackIter != this->onetimeCallbacks.end()) {
          if (onetimeCallbackIter->second) {
            onetimeCallbackIter->second();
          }

          if (callbacksAreValid) {
            onetimeCallbacks.erase(onetimeCallbackIter);
          }
        }

        // If the playback mode ws set to loop...
        if ((playbackMode & Mode::Loop) == Mode::Loop && progress > 0.f && &(*iter) == &copy.back()) {
          // But it was also set to bounce, reverse the list and start over
          if ((playbackMode & Mode::Bounce) == Mode::Bounce) {
            reverse(copy.begin(), copy.end());
            iter = copy.begin();
            iter++; // last frame _was_ first frame for reversed animations, move to the 2nd
          }
          else {
            // It was set only to loop, start from the beginning
            iter = copy.begin();
          }

          // Clear any remaining callbacks
          this->callbacks.clear();

          // Enqueue the callbacks for the next go around
          this->callbacks = nextLoopCallbacks;
          this->nextLoopCallbacks.clear();

          callbacksAreValid = true;

          continue; // Start loop again
        }

        // Apply the frame to the sprite object
        target.setTextureRect((*iter).subregion);

        // If applicable, apply the origin too
        if ((*iter).applyOrigin) {
          target.setOrigin((float)(*iter).origin.x, (float)(*iter).origin.y);
        }

        UpdateCurrentPoints(index-1, sequence);

        break;
      }

      // If not finish, go to next frame
      iter++;
  }

  // If we prematurely ended the loop, update the sprite
  if (iter != copy.end()) {
    target.setTextureRect((*iter).subregion);

    // If applicable, update the origin
    if ((*iter).applyOrigin) {
      target.setOrigin((float)(*iter).origin.x, (float)(*iter).origin.y);
    }
  }

  // End updating flag
  isUpdating = false;
  callbacksAreValid = true;

  UpdateCurrentPoints((index==0)? 0 : index-1, sequence);

  // Merge queued callbacks
  callbacks.insert(queuedCallbacks.begin(), queuedCallbacks.end());
  queuedCallbacks.clear();

  onetimeCallbacks.insert(queuedOnetimeCallbacks.begin(), queuedOnetimeCallbacks.end());
  queuedOnetimeCallbacks.clear();

  if (queuedOnFinish) {
    onFinish = queuedOnFinish;
    queuedOnFinish = nullptr;
  }
}

/**
 * @brief Add a callback 
 * @param rhs On struct
 * @return Animator& to chain
 * 
 * If in the middle of an update loop, add to the queue otherwise add directly to callback list
 */
Animator & Animator::operator<<(On rhs)
{
  if(!rhs.callback) return *this;
  
  if (rhs.doOnce) {
	  if(this->isUpdating) {
		  this->queuedOnetimeCallbacks.insert(std::make_pair(rhs.id, rhs.callback));
	  } else {
          this->onetimeCallbacks.insert(std::make_pair(rhs.id, rhs.callback));
      }
  }
  else {
	if(this->isUpdating) {
		// std:: cout << "callback queued" << std::endl;
        this->queuedCallbacks.insert(std::make_pair(rhs.id, rhs.callback));
    } else {
        this->callbacks.insert(std::make_pair(rhs.id, rhs.callback));		
	}
  }

  return *this;
}

Animator & Animator::operator<<(char rhs)
{
  this->playbackMode = rhs;
  return *this;
}

void Animator::operator<<(std::function<void()> finishNotifier)
{
  if(!finishNotifier) return;
  
  if(!this->isUpdating) {
    this->onFinish = finishNotifier;
  } else {
	  this->queuedOnFinish = finishNotifier;
  }
}

void Animator::SetFrame(int frameIndex, sf::Sprite & target, FrameList& sequence)
{
  int index = 0;
  for (Frame& frame : sequence.frames) {
    index++;

    if (index == frameIndex) {
      target.setTextureRect(frame.subregion);
      if (frame.applyOrigin) {
        target.setOrigin((float)frame.origin.x, (float)frame.origin.y);
      }
      
      UpdateCurrentPoints(frameIndex-1, sequence);

      return;
    }
  }

  Logger::Log("finished without applying frame. Frame sizes: " + std::to_string(sequence.frames.size()));

}
