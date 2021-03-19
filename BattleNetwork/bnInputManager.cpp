#include <SFML/Window.hpp>
#include <SFML/Window/Clipboard.hpp>
using sf::Event;
using sf::Keyboard;
#include "bnGame.h"
#include "bnInputManager.h"

#if defined(__ANDROID__)
#include "Android/bnTouchArea.h"
#endif

// #include <iostream>

#define GAMEPAD_AXIS_SENSITIVITY 30.f

InputManager::InputManager(sf::Window& win)  : 
  window(win),
  settings() {
  lastkey = sf::Keyboard::Key::Unknown;
  lastButton = (decltype(lastButton))-1;
  lastAxisXPower = axisXPower = lastAxisYPower = axisYPower = 0.f;
}


InputManager::~InputManager() {
}

void InputManager::SupportConfigSettings(ConfigReader& reader) {
  settings = reader.GetConfigSettings();
  events.clear();
  eventsLastFrame.clear();
}

void InputManager::Update() {
  eventsLastFrame = events;
  events.clear();
  systemCopyEvent = systemPasteEvent = false;

  Event event;

  lastkey = sf::Keyboard::Key::Unknown;
  lastButton = (decltype(lastButton))-1;

  while (window.pollEvent(event)) {
    if (event.type == Event::Closed) {
      onLoseFocus();
      window.close();
      hasFocus = false;
    } else if (event.type == Event::LostFocus) {
      onLoseFocus();
      hasFocus = false;

      FlushAllInputEvents();
    }
    else if (event.type == Event::GainedFocus) {
      onRegainFocus();
      hasFocus = true;

      // we have re-entered, do not let keys be held down
      FlushAllInputEvents();
    }
    else if (event.type == Event::Resized) {
      onResized(event.size.width, event.size.height);
      hasFocus = true;
    }

    if (event.type == sf::Event::TextEntered && captureInputBuffer) {
      HandleInputBuffer(event);
    }

    if(event.type == sf::Event::EventType::KeyPressed) {
      if (!hasFocus) continue;

      lastkey = event.key.code;

#ifndef __ANDROID__
      if (event.key.control && event.key.code == sf::Keyboard::V)
        systemPasteEvent = true;

      if (event.key.control && event.key.code == sf::Keyboard::C)
        systemCopyEvent = true;
#endif
    }

    // keep the gamepad list up-to-date
    gamepads.clear();

    for (unsigned int i = 0; i < sf::Joystick::Count; i++) {
      if (sf::Joystick::isConnected(i)) {
        gamepads.push_back(sf::Joystick::Identification());
      }
    }

    for (unsigned int i = 0; i < sf::Joystick::getButtonCount(currGamepad) && hasFocus && IsUsingGamepadControls(); i++) {
      if (sf::Joystick::isButtonPressed(currGamepad, i)) {
        lastButton = (decltype(lastButton))i;
      }
    }

    if (sf::Joystick::isConnected(currGamepad) && settings.IsOK() && hasFocus && IsUsingGamepadControls()) {
      for (unsigned int i = 0; i < sf::Joystick::getButtonCount(currGamepad); i++) {
        if (sf::Joystick::isButtonPressed(currGamepad, i)) {
          auto action = settings.GetPairedActions((Gamepad)i);

          for (auto a : action) {
            events.push_back({ a, InputState::pressed });
          }
          
        } else {
          /*
          joysticks can only determine if the signal is on or off,
          we must compare with the last frame to determine if this 
          was a release event
          */
          auto action = settings.GetPairedActions((Gamepad)i);

          for (auto a : action) {
            bool canRelease = false;

            InputEvent find1 = { a, InputState::held };
            InputEvent find2 = { a, InputState::pressed };

            if (find(eventsLastFrame.begin(), eventsLastFrame.end(), find1) != eventsLastFrame.end()) {
              canRelease = true;
            }
            else if (find(eventsLastFrame.begin(), eventsLastFrame.end(), find2) != eventsLastFrame.end()) {
              canRelease = true;
            }       

            if (canRelease) {
              events.push_back({ a, InputState::released });
            }
          }
        }
      }
    } else if (Event::EventType::KeyPressed == event.type && hasFocus && IsUsingKeyboardControls()) {
      /* Gamepad not connected. Strictly use keyboard events. */
      if (settings.IsOK() && settings.IsKeyboardOK()) {
        auto action = settings.GetPairedActions(event.key.code);

        for (auto a : action) {
          events.push_back({ a, InputState::pressed });
        }
      } else {
        if (Keyboard::Up == event.key.code) {
          events.push_back(InputEvents::pressed_move_up);
          events.push_back(InputEvents::pressed_ui_up);
        }
        else if (Keyboard::Left == event.key.code) {
          events.push_back(InputEvents::pressed_move_left);
          events.push_back(InputEvents::pressed_ui_left);
        }
        else if (Keyboard::Down == event.key.code) {
          events.push_back(InputEvents::pressed_move_down);
          events.push_back(InputEvents::pressed_ui_down);
        }
        else if (Keyboard::Right == event.key.code) {
          events.push_back(InputEvents::pressed_move_right);
          events.push_back(InputEvents::pressed_ui_right);
        }
        else if (Keyboard::X == event.key.code) {
          events.push_back(InputEvents::pressed_cancel);
          events.push_back(InputEvents::pressed_shoot);
          events.push_back(InputEvents::pressed_run);
        }
        else if (Keyboard::Z == event.key.code) {
          events.push_back(InputEvents::pressed_confirm);
          events.push_back(InputEvents::pressed_use_chip);
          events.push_back(InputEvents::pressed_interact);
        }
        else if (Keyboard::Space == event.key.code) {
          events.push_back(InputEvents::pressed_cust_menu);
          events.push_back(InputEvents::pressed_option);
        }
        else if (Keyboard::P == event.key.code) {
          events.push_back(InputEvents::pressed_pause);
        }
        else if (Keyboard::A == event.key.code) {
          events.push_back(InputEvents::pressed_cust_menu);
        }
        else if (Keyboard::S == event.key.code) {
          events.push_back(InputEvents::pressed_special);
        }
        else if (Keyboard::D == event.key.code) {
          events.push_back(InputEvents::pressed_scan_left);
        }
        else if (Keyboard::F == event.key.code) {
          events.push_back(InputEvents::pressed_scan_right);
        }
      }
    } else if (Event::KeyReleased == event.type && hasFocus && IsUsingKeyboardControls()) {
      if (settings.IsOK() && settings.IsKeyboardOK()) {
        auto action = settings.GetPairedActions(event.key.code);

        if (!action.size()) continue;

        for (auto a : action) {
          events.push_back({ a, InputState::released });
        }
      }
      else {
        if (Keyboard::Up == event.key.code) {
          events.push_back(InputEvents::released_move_up);
          events.push_back(InputEvents::released_ui_up);
        }
        else if (Keyboard::Left == event.key.code) {
          events.push_back(InputEvents::released_move_left);
          events.push_back(InputEvents::released_ui_left);
        }
        else if (Keyboard::Down == event.key.code) {
          events.push_back(InputEvents::released_move_down);
          events.push_back(InputEvents::released_ui_down);
        }
        else if (Keyboard::Right == event.key.code) {
          events.push_back(InputEvents::released_move_right);
          events.push_back(InputEvents::released_ui_right);
        }
        else if (Keyboard::X == event.key.code) {
          events.push_back(InputEvents::released_cancel);
          events.push_back(InputEvents::released_shoot);
          events.push_back(InputEvents::released_run);
        }
        else if (Keyboard::Z == event.key.code) {
          events.push_back(InputEvents::released_confirm);
          events.push_back(InputEvents::released_use_chip);
          events.push_back(InputEvents::released_interact);
        }
        else if (Keyboard::Space == event.key.code) {
          events.push_back(InputEvents::released_cust_menu);
          events.push_back(InputEvents::released_option);
        }
        else if (Keyboard::P == event.key.code) {
          events.push_back(InputEvents::released_pause);
        }
        else if (Keyboard::A == event.key.code) {
          events.push_back(InputEvents::released_cust_menu);
        }
        else if (Keyboard::S == event.key.code) {
          events.push_back(InputEvents::released_special);
        }
        else if (Keyboard::D == event.key.code) {
          events.push_back(InputEvents::released_scan_left);
        }
        else if (Keyboard::F == event.key.code) {
          events.push_back(InputEvents::released_scan_right);
        }
      }
    }
  } // end event poll

  // Check these every frame regardless of input state...
  lastAxisXPower = axisXPower;
  lastAxisYPower = axisYPower;

  axisXPower = 0.f;
  axisYPower = 0.f;

  if (sf::Joystick::isConnected(currGamepad) && hasFocus && IsUsingGamepadControls()) {

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
      axisYPower += -sf::Joystick::getAxisPosition(currGamepad, sf::Joystick::Axis::Y);
    }

    if (axisXPower <= -GAMEPAD_AXIS_SENSITIVITY) {
      lastButton = Gamepad::LEFT;

      auto action = settings.GetPairedActions((Gamepad)lastButton);

      for (auto a : action) {
        events.push_back({ a, InputState::pressed });
      }
    }
    
    if (axisXPower >= GAMEPAD_AXIS_SENSITIVITY) {
      lastButton = Gamepad::RIGHT;

      auto action = settings.GetPairedActions((Gamepad)lastButton);

      for (auto a : action) {
        events.push_back({ a, InputState::pressed });
      }
    }

    if (axisYPower >= GAMEPAD_AXIS_SENSITIVITY) {
      lastButton = Gamepad::UP;

      auto action = settings.GetPairedActions((Gamepad)lastButton);

      for (auto a : action) {
        events.push_back({ a, InputState::pressed });
      }
    }

    if (axisYPower <= -GAMEPAD_AXIS_SENSITIVITY) {
      lastButton = Gamepad::DOWN;

      auto action = settings.GetPairedActions((Gamepad)lastButton);

      for (auto a : action) {
        events.push_back({ a, InputState::pressed });
      }
    }

    if (axisXPower - lastAxisXPower != 0.f) {
      if (axisXPower > -GAMEPAD_AXIS_SENSITIVITY && lastAxisXPower <= -GAMEPAD_AXIS_SENSITIVITY) {
        auto action = settings.GetPairedActions(Gamepad::LEFT);

        for (auto a : action) {
          events.push_back({ a, InputState::released });
        }
      }

      if (axisXPower < GAMEPAD_AXIS_SENSITIVITY && lastAxisXPower >= GAMEPAD_AXIS_SENSITIVITY) {
        auto action = settings.GetPairedActions(Gamepad::RIGHT);

        for (auto a : action) {
          events.push_back({ a, InputState::released });
        }
      }
    }

    if (axisYPower - lastAxisYPower != 0.f) {
      if (axisYPower > -GAMEPAD_AXIS_SENSITIVITY && lastAxisYPower <= -GAMEPAD_AXIS_SENSITIVITY) {
        auto action = settings.GetPairedActions(Gamepad::DOWN);

        for (auto a : action) {
          events.push_back({ a, InputState::released });
        }
      }

      if (axisYPower < GAMEPAD_AXIS_SENSITIVITY && lastAxisYPower >= GAMEPAD_AXIS_SENSITIVITY) {
        auto action = settings.GetPairedActions(Gamepad::UP);

        for (auto a : action) {
          events.push_back({ a, InputState::released });
        }
      }
    }
  }

  // First, we must see if any InputState::held keys from the last frame are released this frame
  auto eventEraseThunk=[](std::vector<InputEvent>& container, InputState if_value, InputState erase_value) {
    for (auto iter = container.begin(); iter != container.end();) {
      InputEvent e = *iter;

      if (e.state == if_value) {
        auto trunc = std::remove(container.begin(), container.end(), InputEvent{ e.name, erase_value });
        size_t sz = std::distance(trunc, container.end());
        container.erase(trunc, container.end());

        if (sz > 0) {
          iter = container.begin(); // start over now that we changed the order of the container
        }
      }

      iter++;
    }
  };

  eventEraseThunk(eventsLastFrame, InputState::released, InputState::pressed);
  eventEraseThunk(eventsLastFrame, InputState::released, InputState::held);
  eventEraseThunk(events, InputState::released, InputState::pressed);
  eventEraseThunk(events, InputState::released, InputState::held);

  // Uncomment for debugging
  /*for (InputEvent e : eventsLastFrame) {
    std::string state = "NONE";

    switch (e.state) {
    case InputState::held:
      state = "InputState::held";
      break;
    case InputState::released:
      state = "InputState::released";
      break;
    case InputState::pressed:
      state = "InputState::pressed";
      break;
    }
    Logger::Logf("input event %s, %s", e.name.c_str(), state.c_str());
  }*/

  // Finally, merge the continuous InputState::held key events into the final event buffer
  for (InputEvent e : eventsLastFrame) {
    InputEvent insert = InputEvents::none;
    InputEvent erase  = InputEvents::none;

      // Search for the InputState::held events to migrate into the current event frame
      // Prevent further events if a key is InputState::held
    if (e.state == InputState::held) {
      insert = e;
      erase = { e.name, InputState::pressed };
    }
   
    // Search for press keys that have been InputState::held and transform them
    if (e.state == InputState::pressed) {
      insert = { e.name, InputState::held };
      erase = e;
    }
   
    auto trunc = std::remove(events.begin(), events.end(), erase);
    events.erase(trunc, events.end());

    if (insert != InputEvents::none && std::find(events.begin(), events.end(), insert) == events.end()) {
      events.push_back(insert); // migrate this input
    }
  }

  eventsLastFrame.clear();

#ifdef __ANDROID__
    events.clear(); // TODO: what inputs get stuck in the event list on droid?
    TouchArea::poll();
#endif
}

sf::Keyboard::Key InputManager::GetAnyKey()
{
  return lastkey;
}

std::string InputManager::GetClipboard()
{
  return sf::Clipboard::getString();
}

void InputManager::SetClipboard(const std::string& data)
{
  sf::Clipboard::setString(sf::String(data));
}

Gamepad InputManager::GetAnyGamepadButton()
{
  return lastButton;
}

const bool InputManager::ConvertKeyToString(const sf::Keyboard::Key key, std::string & out)
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
    case sf::Keyboard::Key::LAlt:
      out = std::string("L ALT"); return true;
    case sf::Keyboard::Key::RAlt:
      out = std::string("R ALT"); return true;
    case sf::Keyboard::Key::RControl:
      out = std::string("R CTRL"); return true;
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

bool InputManager::Has(InputEvent _event) {
  return events.end() != find(events.begin(), events.end(), _event);
}

void InputManager::VirtualKeyEvent(InputEvent event) {
  events.push_back(event);
}

void InputManager::BindRegainFocusEvent(std::function<void()> callback)
{
  onRegainFocus = callback;
}

void InputManager::BindResizedEvent(std::function<void(int, int)> callback)
{
  onResized = callback;
}

void InputManager::BindLoseFocusEvent(std::function<void()> callback)
{
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
  return !useGamepadControls;
}

ConfigSettings InputManager::GetConfigSettings()
{
  return settings;
}

void InputManager::FlushAllInputEvents()
{
  for (auto e : eventsLastFrame) {
    InputEvent insert = InputEvents::none;
    InputEvent erase = InputEvents::none;

    // Search for press keys that have been InputState::held and transform them
    if (e.state == InputState::pressed || e.state == InputState::held) {
      insert = { e.name, InputState::released };
      erase = e;
    }

    auto trunc = std::remove(events.begin(), events.end(), erase);
    events.erase(trunc, events.end());

    if (insert != InputEvents::none && std::find(events.begin(), events.end(), insert) == events.end()) {
      events.push_back(insert); // migrate this release event
    }
  }
}

bool InputManager::Empty() {
  return events.empty();
}

bool InputManager::IsConfigFileValid()
{
  return settings.IsOK();
}

void InputManager::BeginCaptureInputBuffer()
{
  captureInputBuffer = true;
}

void InputManager::EndCaptureInputBuffer()
{
  inputBuffer.clear();
  captureInputBuffer = false;
}

void InputManager::UseKeyboardControls()
{
  useGamepadControls = false;
}

void InputManager::UseGamepadControls()
{
  useGamepadControls = true;
}

void InputManager::UseGamepad(size_t index)
{
  currGamepad = static_cast<unsigned int>(index);
}

const size_t InputManager::GetGamepadCount() const
{
  return gamepads.size();
}

const std::string InputManager::GetInputBuffer()
{
  return inputBuffer;
}

void InputManager::HandleInputBuffer(sf::Event e) {
  if ((e.type == e.KeyPressed && e.key.code == sf::Keyboard::BackSpace) || (e.text.unicode == 8 && inputBuffer.size() != 0)) {
    inputBuffer.pop_back();
  } else if(e.text.unicode < 128 && e.text.unicode != 8) {
      //std::cout << e.text.unicode << std::endl;

#ifdef __ANDROID__
      if(e.text.unicode == 10) {
        VirtualKeyEvent(InputEvent::RELEASED_B);
        return;
      }
#endif

      inputBuffer.push_back((char)e.text.unicode);
  }
}

void InputManager::SetInputBuffer(std::string buff)
{
  inputBuffer = buff;
}
