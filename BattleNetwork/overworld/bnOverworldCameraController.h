
#pragma once

#include <queue>
#include <Swoosh/Timer.h>
#include "../bnCamera.h"

namespace Overworld {
  class CameraController {
  public:
    CameraController();

    bool IsLocked();
    bool IsQueueEmpty();

    /**
    * @brief Stops the camera from following the player
    * @param actor
    */
    void LockCamera();

    /**
     * @brief Locks the camera and queues PlaceCamera
     * @param position
     * @param holdTime
     */
    void QueuePlaceCamera(sf::Vector2f position, sf::Time holdTime = sf::Time::Zero);

    /**
     * @brief Locks the camera and queues MoveCamera
     * @param position
     * @param duration
     */
    void QueueMoveCamera(sf::Vector2f position, sf::Time duration);

    /*
     * @brief Locks the camera and queues WaneCamera
     * @param position
     * @param duration
     * @param waneFactor
     */
    void QueueWaneCamera(sf::Vector2f position, sf::Time duration, double waneFactor);

    /**
     * @brief Camera shakes
     * @param stress intensity of the shake effect
     * @param duration duration of the effect, does not block the queue
     */
    void QueueShakeCamera(float stress, sf::Time duration);

    /**
     * @brief Unlocks the camera and clears the queue after completing previous camera events
     */
    void QueueUnlockCamera();

    /**
     * @brief Clears the camera queue, locks the camera, and moves the camera
     * @param position
     */
    void MoveCamera(sf::Vector2f position);

    /**
     * @brief Clears the camera queue and follow the player again
     */
    void UnlockCamera();

    void UpdateCamera(float elapsed, Camera& camera);

  private:
    enum class CameraEventType { Place, Move, Wane, Shake, Unlock };
    struct CameraEvent {
      CameraEventType type;
      sf::Vector2f position;
      sf::Time duration;
      double intensity;
    };

    swoosh::Timer cameraTimer;
    bool cameraLocked{  };
    std::queue<CameraEvent> queue;
  };
}