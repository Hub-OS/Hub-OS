#include "bnOverworldCameraController.h"

namespace Overworld {
  CameraController::CameraController() {
    cameraTimer.reverse(true);
    cameraTimer.start();
  }

  bool CameraController::IsLocked() {
    return cameraLocked;
  }

  bool CameraController::IsQueueEmpty() {
    return queue.empty();
  }

  void CameraController::LockCamera() {
    cameraLocked = true;
  }

  void CameraController::UnlockCamera() {
    cameraLocked = false;
    queue = {};
  }

  void CameraController::QueuePlaceCamera(sf::Vector2f position, sf::Time holdTime) {
    LockCamera();

    CameraEvent event{
      CameraEventType::Place,
      position,
      holdTime
    };

    queue.push(event);
  }

  void CameraController::QueueMoveCamera(sf::Vector2f position, sf::Time duration) {
    LockCamera();

    CameraEvent event{
      CameraEventType::Move,
      position,
      duration
    };

    queue.push(event);
  }

  void CameraController::QueueWaneCamera(sf::Vector2f position, sf::Time duration, double waneFactor)
  {
    LockCamera();

    CameraEvent event{
      CameraEventType::Wane,
      position,
      duration,
      waneFactor
    };

    queue.push(event);
  }

  void CameraController::QueueShakeCamera(float stress, sf::Time duration) {
    CameraEvent event{
      CameraEventType::Shake,
      sf::Vector2f(),
      duration,
      stress
    };

    queue.push(event);
  }

  void CameraController::QueueUnlockCamera() {
    LockCamera();

    CameraEvent event{
      CameraEventType::Unlock,
    };

    queue.push(event);
  }

  void CameraController::MoveCamera(sf::Vector2f position) {
    queue = {};
    QueueMoveCamera(position, sf::Time::Zero);
  }

  void CameraController::UpdateCamera(float elapsed, Camera& camera) {
    cameraTimer.update(sf::seconds(elapsed));

    if (cameraTimer.getElapsed().asMilliseconds() > 0 || queue.empty()) {
      return;
    }

    auto& event = queue.front();

    switch (event.type) {
    case CameraEventType::Move:
      camera.MoveCamera(event.position, event.duration);
      break;
    case CameraEventType::Wane:
      camera.WaneCamera(event.position, event.duration, event.intensity);
      break;
    case CameraEventType::Place:
      camera.PlaceCamera(event.position);
      break;
    case CameraEventType::Shake:
      camera.ShakeCamera(event.intensity, event.duration);
      // shake duration does not block queue (will still eat a frame, at least for now)
      event.duration = sf::Time::Zero;
      break;
    case CameraEventType::Unlock:
      cameraLocked = false;
      break;
    }

    cameraTimer.set(event.duration);
    queue.pop();
  }
}
