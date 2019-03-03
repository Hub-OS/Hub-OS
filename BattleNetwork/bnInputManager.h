#pragma once
#include <vector>
using std::vector;
#include <map>
using std::map;

#include "bnInputEvent.h"
#include "bnChronoXConfigReader.h"

class InputManager {
public:
  static InputManager& GetInstance();
  ~InputManager();
  void Update();
  bool Has(InputEvent _event);
  bool Empty();
  void SupportChronoXGamepad(ChronoXConfigReader& config);
  bool HasChronoXGamepadSupport();
  void BeginCaptureInputBuffer();
  void EndCaptureInputBuffer();
  const std::string GetInputBuffer();
  void HandleInputBuffer(sf::Event);
  void SetInputBuffer(std::string buff);

private:
  bool captureInputBuffer;
  std::string inputBuffer;

  InputManager();
  vector<InputEvent> events;
  vector<InputEvent> eventsLastFrame;

  map<std::string, bool> gamepadPressed;

  // Support for ChronoX config.ini files
  ChronoXConfigReader* config;
};

#define INPUT InputManager::GetInstance()