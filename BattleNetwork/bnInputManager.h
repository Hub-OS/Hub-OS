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
  void update();
  bool has(InputEvent _event);
  bool empty();
  void SupportChronoXGamepad(ChronoXConfigReader& config);
  bool HasChronoXGamepadSupport();
  void VirtualKeyEvent(InputEvent event);
private:
  InputManager();
  vector<InputEvent> events;
  map<std::string, bool> gamepadPressed;

  // Support for ChronoX config.ini files
  ChronoXConfigReader* config;
};

#define INPUT InputManager::GetInstance()