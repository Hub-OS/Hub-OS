#include <SFML/Window.hpp>
#include <SFML/Window/Clipboard.hpp>
#include <mutex>

#include "bnGame.h"
#include "bnInputManager.h"

#if defined(__ANDROID__)
#include "Android/bnTouchArea.h"
#endif

using sf::Event;
using sf::Keyboard;

#define GAMEPAD_AXIS_SENSITIVITY 30.f

InputManager::InputManager(sf::Window& win)  : 
  window(win),
  settings(),
  textBuffer() {
  queuedLastKey = lastkey = sf::Keyboard::Key::Unknown;
  queuedLastButton = lastButton = (decltype(lastButton))-1;
  queuedKeyboardState.fill(false);
  keyboardState.fill(false);
}

InputManager::~InputManager() {
}

void InputManager::SupportConfigSettings(ConfigSettings& settings) {
  this->settings = settings;
  state.clear();
  stateLastFrame.clear();
  bindings.clear();

  std::unordered_map<std::string, std::vector<Binding>> intermediateBindings;

  // store keyboard bindings
  for (auto& [input, name] : settings.GetKeyboardHash()) {
    if (input < 0 || input >= keyboardState.size()) {
      // Key::Unknown (no binding), or invalid key
      continue;
    }

    Binding binding;
    binding.input = input;
    binding.isKeyboardBinding = true;

    intermediateBindings[name].push_back(binding);
  }

  // store controller bindings
  for (auto& [input, name] : settings.GetGamepadHash()) {
    if ((int)input < 0) {
      // invalid/no binding
      continue;
    }

    Binding binding;
    binding.input = static_cast<unsigned int>(input);

    intermediateBindings[name].push_back(binding);
  }

  for (auto& [name, actionBindings] : intermediateBindings) {
    bindings.push_back({ name, actionBindings });
  }

  currGamepad = settings.GetGamepadIndex();
  invertThumbstick = settings.GetInvertThumbstick();
}

void InputManager::Update()
{
  this->mutex.lock();

  // Prioritize press events only (discard release events)
  for (auto iter = queuedState.begin(); iter != queuedState.end(); /* skip */) {
    auto& event = *iter;

    if (event.second == InputState::pressed) {
      auto query = [event](const std::pair<std::string, InputState>& pair) {
        return pair.first == event.first && pair.second == InputState::released;
      };

      auto iterFind = std::find_if(queuedState.begin(), queuedState.end(), query);

      if (iterFind != queuedState.end()) {
        iter = queuedState.erase(iterFind);
        continue;
      }
    }

    iter++;
  }

  std::swap(queuedLastKey, lastkey);
  std::swap(queuedLastButton, lastButton);
  std::swap(stateLastFrame, state); // (store state in stateLastFrame, without deep copy)
  std::swap(queuedSystemCopyEvent, systemCopyEvent);
  std::swap(queuedSystemPasteEvent, systemPasteEvent);

  state.clear();
  queuedSystemCopyEvent = queuedSystemPasteEvent = false;
  queuedLastKey = sf::Keyboard::Key::Unknown;
  queuedLastButton = static_cast<Gamepad>(-1);

  // Uncomment for debugging
/*for (auto& [name, state] : stateLastFrame) {
  std::string stateName = "NONE";

  switch (state) {
  case InputState::held:
    stateName = "InputState::held";
    break;
  case InputState::released:
    stateName = "InputState::released";
    break;
  case InputState::pressed:
    stateName = "InputState::pressed";
    break;
  }
  Logger::Logf("input event %s, %s", name.c_str(), stateName.c_str());
}*/

// Finally, merge the continuous InputState::held key events into the final event buffer
  for (auto& [name, value]: queuedState) {
    auto previouslyHeld = (value == InputState::pressed && stateLastFrame[name] == InputState::pressed)|| stateLastFrame[name] == InputState::held;

    if (previouslyHeld) {
      state[name] = InputState::held;
    }
    else{
      state[name] = value;
    }
  }

  for (auto& [name, value]: stateLastFrame) {
    if (queuedState[name] == InputState::none && value != InputState::released) {
      state[name] = InputState::released;
    }
  }

  queuedState.clear();

#ifdef __ANDROID__
  state.clear(); // TODO: what inputs get stuck in the event list on droid?
  TouchArea::poll();
#endif
  this->mutex.unlock();
}

void InputManager::EventPoll() {
  Event event{};

  while (window.pollEvent(event)) {
    if (event.type == Event::Closed) {
      onLoseFocus ? onLoseFocus() : (void)0;
      window.close();
      hasFocus = false;
    } else if (event.type == Event::LostFocus) {
      onLoseFocus ? onLoseFocus() : (void)0;
      hasFocus = false;

      FlushAllInputEvents();
    }
    else if (event.type == Event::GainedFocus) {
      onRegainFocus ? onRegainFocus() : (void)0;
      hasFocus = true;

      // we have re-entered, do not let keys be held down
      FlushAllInputEvents();
    }
    else if (event.type == Event::Resized) {
      onResized? onResized(event.size.width, event.size.height) : (void)0;
      hasFocus = true;
    }
    else if (event.type == sf::Event::TextEntered) {
      textBuffer.HandleTextEntered(event);
    } else if(event.type == sf::Event::EventType::KeyPressed) {
      if (hasFocus) {
        queuedLastKey = event.key.code;

#ifndef __ANDROID__
        if (event.key.control && event.key.code == sf::Keyboard::V)
          queuedSystemPasteEvent = true;

        if (event.key.control && event.key.code == sf::Keyboard::C)
          queuedSystemCopyEvent = true;
#endif

        textBuffer.HandleKeyPressed(event);
      }
    }

    if (event.type == Event::EventType::KeyPressed) {
      if(event.key.code != Keyboard::Unknown) {
        queuedKeyboardState[event.key.code] = true;
      }
    } else if (event.type == Event::KeyReleased) {
      if(event.key.code != Keyboard::Unknown) {
        queuedKeyboardState[event.key.code] = false;
      }
    }
  } // end event poll

  if (queuedSystemPasteEvent) {
    textBuffer.HandlePaste(GetClipboard());
  }

  textBuffer.HandleCompletedEventProcessing();

  // keep the gamepad list up-to-date
  gamepads.clear();

  for (unsigned int i = 0; i < sf::Joystick::Count; i++) {
    if (sf::Joystick::isConnected(i)) {
      gamepads.push_back(sf::Joystick::Identification());
    }
  }

  this->mutex.lock();

  // update gamepad inputs
  if (sf::Joystick::isConnected(currGamepad)) {
    queuedGamepadState.clear();

    // track buttons
    for (unsigned int i = 0; i < sf::Joystick::getButtonCount(currGamepad); i++) {
      auto buttonPressed = sf::Joystick::isButtonPressed(currGamepad, i);

      queuedGamepadState[i] = buttonPressed;

      if (buttonPressed) {
        queuedLastButton = (decltype(queuedLastButton))i;
      }
    }

    // track axes
    float axisXPower = 0.f;
    float axisYPower = 0.f;

    if (sf::Joystick::hasAxis(currGamepad, sf::Joystick::PovX)) {
      axisXPower = sf::Joystick::getAxisPosition(currGamepad, sf::Joystick::PovX);
    }

    if (sf::Joystick::hasAxis(currGamepad, sf::Joystick::PovY)) {
      axisYPower = sf::Joystick::getAxisPosition(currGamepad, sf::Joystick::PovY);
    }

    if (sf::Joystick::hasAxis(currGamepad, sf::Joystick::Axis::X)) {
      axisXPower += sf::Joystick::getAxisPosition(currGamepad, sf::Joystick::Axis::X);
    }

    if (sf::Joystick::hasAxis(currGamepad, sf::Joystick::Axis::Y)) {
      axisYPower += sf::Joystick::getAxisPosition(currGamepad, sf::Joystick::Axis::Y) * (invertThumbstick ? -1 : 1);
    }

    if (axisXPower <= -GAMEPAD_AXIS_SENSITIVITY) {
      queuedLastButton = Gamepad::LEFT;
      queuedGamepadState[(unsigned int)queuedLastButton] = true;
    }
    else if (axisXPower >= GAMEPAD_AXIS_SENSITIVITY) {
      queuedLastButton = Gamepad::RIGHT;
      queuedGamepadState[(unsigned int)queuedLastButton] = true;
    }

    if (axisYPower >= GAMEPAD_AXIS_SENSITIVITY) {
      queuedLastButton = Gamepad::UP;
      queuedGamepadState[(unsigned int)queuedLastButton] = true;
    }
    else if (axisYPower <= -GAMEPAD_AXIS_SENSITIVITY) {
      queuedLastButton = Gamepad::DOWN;
      queuedGamepadState[(unsigned int)queuedLastButton] = true;
    }
  }

  if (hasFocus) {
    if (settings.IsOK()) {
      for (auto& [name, actionBindings] : bindings) {
        bool isActive = false;

        for (auto& binding : actionBindings) {
          isActive |=
            binding.isKeyboardBinding
              ? useKeyboardControls && queuedKeyboardState[binding.input]
              : useGamepadControls && queuedGamepadState[binding.input];

          if (isActive) {
            break;
          }
        }

        if(isActive) {
          queuedState[name] = InputState::pressed;
        }
      }
    }
    else {
      if (keyboardState[sf::Keyboard::Key::Up]) {
        VirtualKeyEvent(InputEvents::pressed_move_up);
        VirtualKeyEvent(InputEvents::pressed_ui_up);
      }
      if (keyboardState[sf::Keyboard::Key::Left]) {
        VirtualKeyEvent(InputEvents::pressed_move_left);
        VirtualKeyEvent(InputEvents::pressed_ui_left);
      }
      if (keyboardState[sf::Keyboard::Key::Down]) {
        VirtualKeyEvent(InputEvents::pressed_move_down);
        VirtualKeyEvent(InputEvents::pressed_ui_down);
      }
      if (keyboardState[sf::Keyboard::Key::Right]) {
        VirtualKeyEvent(InputEvents::pressed_move_right);
        VirtualKeyEvent(InputEvents::pressed_ui_right);
      }
      if (keyboardState[sf::Keyboard::Key::X]) {
        VirtualKeyEvent(InputEvents::pressed_cancel);
        VirtualKeyEvent(InputEvents::pressed_use_chip);
        VirtualKeyEvent(InputEvents::pressed_run);
      }
      if (keyboardState[sf::Keyboard::Key::Z]) {
        VirtualKeyEvent(InputEvents::pressed_shoot);
        VirtualKeyEvent(InputEvents::pressed_confirm);
        VirtualKeyEvent(InputEvents::pressed_interact);
      }
      if (keyboardState[sf::Keyboard::Key::Enter]) {
        VirtualKeyEvent(InputEvents::pressed_pause);
      }
      if (keyboardState[sf::Keyboard::Key::C]) {
        VirtualKeyEvent(InputEvents::pressed_special);
        VirtualKeyEvent(InputEvents::pressed_option);
      }
      if (keyboardState[sf::Keyboard::Key::A]) {
        VirtualKeyEvent(InputEvents::pressed_shoulder_left);
      }
      if (keyboardState[sf::Keyboard::Key::S]) {
        VirtualKeyEvent(InputEvents::pressed_shoulder_right);
        VirtualKeyEvent(InputEvents::pressed_cust_menu);
      }
    }
  }

  this->mutex.unlock();
}

bool InputManager::HasFocus() const
{
  return hasFocus;
}

sf::Keyboard::Key InputManager::GetAnyKey() const
{
  return lastkey;
}

std::string InputManager::GetClipboard() const
{
  return sf::Clipboard::getString();
}

void InputManager::SetClipboard(const std::string& data)
{
  sf::Clipboard::setString(sf::String(data));
}

Gamepad InputManager::GetAnyGamepadButton() const
{
  return lastButton;
}

const std::unordered_map<std::string, InputState> InputManager::StateThisFrame() const
{
  return state;
}

const bool InputManager::ConvertKeyToString(const sf::Keyboard::Key key, std::string & out) const
{
  switch (key) {
    case sf::Keyboard::Key::Num1:
      out = std::string("1"); return true;
    case sf::Keyboard::Key::Num2:
      out = std::string("2"); return true;
    case sf::Keyboard::Key::Num3:
      out = std::string("3"); return true;
    case sf::Keyboard::Key::Num4:
      out = std::string("4"); return true;
    case sf::Keyboard::Key::Num5:
      out = std::string("5"); return true;
    case sf::Keyboard::Key::Num6:
      out = std::string("6"); return true;
    case sf::Keyboard::Key::Num7:
      out = std::string("7"); return true;
    case sf::Keyboard::Key::Num8:
      out = std::string("8"); return true;
    case sf::Keyboard::Key::Num9:
      out = std::string("9"); return true;
    case sf::Keyboard::Key::Num0:
      out = std::string("0"); return true;
    case sf::Keyboard::Key::Escape:
      out = std::string("Escape"); return true;
    case sf::Keyboard::Key::Return:
      out = std::string("Enter"); return true;
    case sf::Keyboard::Key::BackSpace:
      out = std::string("BackSpace"); return true;
    case sf::Keyboard::Key::Space:
      out = std::string("Space"); return true;
    case sf::Keyboard::Key::Left:
      out = std::string("Left"); return true;
    case sf::Keyboard::Key::Down:
      out = std::string("Down"); return true;
    case sf::Keyboard::Key::Right:
      out = std::string("Right"); return true;
    case sf::Keyboard::Key::Up:
      out = std::string("Up"); return true;
    case sf::Keyboard::Key::Delete:
      out = std::string("Delete"); return true;
    case sf::Keyboard::Key::A:
      out = std::string("A"); return true;
    case sf::Keyboard::Key::B:
      out = std::string("B"); return true;
    case sf::Keyboard::Key::C:
      out = std::string("C"); return true;
    case sf::Keyboard::Key::D:
      out = std::string("D"); return true;
    case sf::Keyboard::Key::E:
      out = std::string("E"); return true;
    case sf::Keyboard::Key::F:
      out = std::string("F"); return true;
    case sf::Keyboard::Key::G:
      out = std::string("G"); return true;
    case sf::Keyboard::Key::H:
      out = std::string("H"); return true;
    case sf::Keyboard::Key::I:
      out = std::string("I"); return true;
    case sf::Keyboard::Key::J:
      out = std::string("J"); return true;
    case sf::Keyboard::Key::K:
      out = std::string("K"); return true;
    case sf::Keyboard::Key::L:
      out = std::string("L"); return true;
    case sf::Keyboard::Key::M:
      out = std::string("M"); return true;
    case sf::Keyboard::Key::N:
      out = std::string("N"); return true;
    case sf::Keyboard::Key::O:
      out = std::string("O"); return true;
    case sf::Keyboard::Key::P:
      out = std::string("P"); return true;
    case sf::Keyboard::Key::Q:
      out = std::string("Q"); return true;
    case sf::Keyboard::Key::R:
      out = std::string("R"); return true;
    case sf::Keyboard::Key::S:
      out = std::string("S"); return true;
    case sf::Keyboard::Key::T:
      out = std::string("T"); return true;
    case sf::Keyboard::Key::U:
      out = std::string("U"); return true;
    case sf::Keyboard::Key::V:
      out = std::string("V"); return true;
    case sf::Keyboard::Key::W:
      out = std::string("W"); return true;
    case sf::Keyboard::Key::X:
      out = std::string("X"); return true;
    case sf::Keyboard::Key::Y:
      out = std::string("Y"); return true;
    case sf::Keyboard::Key::Z:
      out = std::string("Z"); return true;
    case sf::Keyboard::Key::Tilde:
      out = std::string("~"); return true;
    case sf::Keyboard::Key::Tab:
      out = std::string("TAB"); return true;
    case sf::Keyboard::Key::LControl:
      out = std::string("L CTRL"); return true;
    case sf::Keyboard::Key::LShift:
      out = std::string("L SHFT"); return true;
    case sf::Keyboard::Key::LAlt:
      out = std::string("L ALT"); return true;
    case sf::Keyboard::Key::RAlt:
      out = std::string("R ALT"); return true;
    case sf::Keyboard::Key::RControl:
      out = std::string("R CTRL"); return true;
    case sf::Keyboard::Key::RShift:
      out = std::string("R SHFT"); return true;
    case sf::Keyboard::Key::SemiColon:
      out = std::string(";"); return true;
    case sf::Keyboard::Key::Equal:
      out = std::string("="); return true;
    case sf::Keyboard::Key::Comma:
      out = std::string("<"); return true;
    case sf::Keyboard::Key::Dash:
      out = std::string("-"); return true;
    case sf::Keyboard::Key::Period:
      out = std::string(">"); return true;
    case sf::Keyboard::Key::Divide:
      out = std::string("?"); return true;
    case sf::Keyboard::Key::LBracket:
      out = std::string("L Bracket"); return true;
    case sf::Keyboard::Key::Slash:
      out = std::string("\\"); return true;
    case sf::Keyboard::Key::RBracket:
      out = std::string("R Bracket"); return true;
    case sf::Keyboard::Key::Quote:
      out = std::string("Quote"); return true;
  }

  out = "";

  return false;
}

// NOTE: This is thread-safe when used in game Update() loop
bool InputManager::Has(InputEvent event) const {
  auto it = state.find(event.name);

  if (it == state.end()) {
    return false;
  }

  return it->second == event.state;
}

void InputManager::VirtualKeyEvent(InputEvent event) {
  queuedState[event.name] = event.state;
}

void InputManager::BindRegainFocusEvent(std::function<void()> callback)
{
  std::lock_guard lock(this->mutex);
  onRegainFocus = callback;
}

void InputManager::BindResizedEvent(std::function<void(int, int)> callback)
{
  std::lock_guard lock(this->mutex);
  onResized = callback;
}

void InputManager::BindLoseFocusEvent(std::function<void()> callback)
{
  std::lock_guard lock(this->mutex);
  onLoseFocus = callback;;
}

const bool InputManager::IsGamepadAvailable() const
{
  return sf::Joystick::isConnected(currGamepad);
}

const bool InputManager::HasSystemCopyEvent() const
{
  return systemCopyEvent;
}

const bool InputManager::HasSystemPasteEvent() const
{
  return systemPasteEvent;
}

const bool InputManager::IsUsingGamepadControls() const
{
  return useGamepadControls;
}

const bool InputManager::IsUsingKeyboardControls() const
{
  return useKeyboardControls;
}

ConfigSettings& InputManager::GetConfigSettings()
{
  return settings;
}

void InputManager::FlushAllInputEvents()
{
  for (auto& [name, previousState] : state) {
    // Search for press keys that have been InputState::held and transform them
    if (previousState == InputState::pressed || previousState == InputState::held) {
      queuedState[name] = InputState::released;
    }
  }

  keyboardState.fill(false);
}

bool InputManager::Empty() const {
  return state.empty();
}

bool InputManager::IsConfigFileValid() const
{
  return settings.IsOK();
}

void InputManager::UseKeyboardControls(bool enable)
{
  useKeyboardControls = enable;
}

void InputManager::UseGamepadControls(bool enable)
{
  useGamepadControls = enable;
}

void InputManager::UseGamepad(size_t index)
{
  currGamepad = static_cast<unsigned int>(index);
}

void InputManager::SetInvertThumbstick(bool invert)
{
  invertThumbstick = invert;
}

const size_t InputManager::GetGamepadCount() const
{
  return gamepads.size();
}

InputTextBuffer& InputManager::GetInputTextBuffer() {
  return textBuffer;
}
