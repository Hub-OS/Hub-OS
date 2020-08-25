#include <SFML/Window.hpp>
#include <SFML/Window/Clipboard.hpp>
using sf::Event;
using sf::Keyboard;
#include "bnEngine.h"
#include "bnInputManager.h"

#if defined(__ANDROID__)
#include "Android/bnTouchArea.h"
#endif

// #include <iostream>

#define GAMEPAD_1 0
#define GAMEPAD_1_AXIS_SENSITIVITY 30.f

InputManager& InputManager::GetInstance() {
  static InputManager instance;
  return instance;
}

InputManager::InputManager()  : settings() {
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

  while (ENGINE.GetWindow()->pollEvent(event)) {
    if (event.type == Event::Closed) {
      onLoseFocus();
      ENGINE.GetWindow()->close();
    }

    if (event.type == Event::LostFocus) {
      onLoseFocus();
    }
    else if (event.type == Event::GainedFocus) {
      onRegainFocus();
    }
    else if (event.type == Event::Resized) {
      onResized(event.size.width, event.size.height);
    }

    if (event.type == sf::Event::TextEntered && captureInputBuffer) {
      HandleInputBuffer(event);
    }

    if(event.type == sf::Event::KeyPressed) {
      lastkey = event.key.code;

#ifndef __ANDROID__
      if (event.key.control && event.key.code == sf::Keyboard::V)
        systemPasteEvent = true;

      if (event.key.control && event.key.code == sf::Keyboard::C)
        systemCopyEvent = true;
#endif
    }

    for (unsigned int i = 0; i < sf::Joystick::getButtonCount(GAMEPAD_1); i++) {
      if (sf::Joystick::isButtonPressed(GAMEPAD_1, i)) {
        lastButton = (decltype(lastButton))i;
      }
    }

    if (sf::Joystick::isConnected(GAMEPAD_1) && settings.IsOK()) {
      for (unsigned int i = 0; i < sf::Joystick::getButtonCount(GAMEPAD_1); i++) {
        if (sf::Joystick::isButtonPressed(GAMEPAD_1, i)) {
          auto action = settings.GetPairedActions((Gamepad)i);

          for (auto a : action) {
            events.push_back({ a, InputState::PRESSED });
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

            InputEvent find1 = { a, InputState::HELD };
            InputEvent find2 = { a, InputState::PRESSED };

            if (find(eventsLastFrame.begin(), eventsLastFrame.end(), find1) != eventsLastFrame.end()) {
              canRelease = true;
            }
            else if (find(eventsLastFrame.begin(), eventsLastFrame.end(), find2) != eventsLastFrame.end()) {
              canRelease = true;
            }       

            if (canRelease) {
              events.push_back({ a, InputState::RELEASED });
            }
          }
        }
      }
    } else if (Event::KeyPressed == event.type) {
      /* Gamepad not connected. Strictly use keyboard events. */
      if (settings.IsOK() && settings.IsKeyboardOK()) {
        auto action = settings.GetPairedActions(event.key.code);

        for (auto a : action) {
          events.push_back({ a, InputState::PRESSED });
        }
      } else {
        if (Keyboard::Up == event.key.code) {
          events.push_back(EventTypes::PRESSED_MOVE_UP);
          events.push_back(EventTypes::PRESSED_UI_UP);
        }
        else if (Keyboard::Left == event.key.code) {
          events.push_back(EventTypes::PRESSED_MOVE_LEFT);
          events.push_back(EventTypes::PRESSED_UI_LEFT);
        }
        else if (Keyboard::Down == event.key.code) {
          events.push_back(EventTypes::PRESSED_MOVE_DOWN);
          events.push_back(EventTypes::PRESSED_UI_DOWN);
        }
        else if (Keyboard::Right == event.key.code) {
          events.push_back(EventTypes::PRESSED_MOVE_RIGHT);
          events.push_back(EventTypes::PRESSED_UI_RIGHT);
        }
        else if (Keyboard::X == event.key.code) {
          events.push_back(EventTypes::PRESSED_CONFIRM);
          events.push_back(EventTypes::PRESSED_USE_CHIP);
        }
        else if (Keyboard::Z == event.key.code) {
          events.push_back(EventTypes::PRESSED_CANCEL);
          events.push_back(EventTypes::PRESSED_SHOOT);
        }
        else if (Keyboard::Space == event.key.code) {
          events.push_back(EventTypes::PRESSED_CUST_MENU);
          events.push_back(EventTypes::PRESSED_QUICK_OPT);
        }
        else if (Keyboard::P == event.key.code) {
          events.push_back(EventTypes::PRESSED_PAUSE);
        }
        else if (Keyboard::A == event.key.code) {
          events.push_back(EventTypes::PRESSED_CUST_MENU);
        }
        else if (Keyboard::S == event.key.code) {
          events.push_back(EventTypes::PRESSED_SPECIAL);
        }
        else if (Keyboard::D == event.key.code) {
          events.push_back(EventTypes::PRESSED_SCAN_LEFT);
        }
        else if (Keyboard::F == event.key.code) {
          events.push_back(EventTypes::PRESSED_SCAN_RIGHT);
        }
      }
    } else if (Event::KeyReleased == event.type) {
      if (settings.IsOK() && settings.IsKeyboardOK()) {
        auto action = settings.GetPairedActions(event.key.code);

        if (!action.size()) continue;

        for (auto a : action) {
          events.push_back({ a, InputState::RELEASED });
        }
      }
      else {
        if (Keyboard::Up == event.key.code) {
          events.push_back(EventTypes::RELEASED_MOVE_UP);
          events.push_back(EventTypes::RELEASED_UI_UP);
        }
        else if (Keyboard::Left == event.key.code) {
          events.push_back(EventTypes::RELEASED_MOVE_LEFT);
          events.push_back(EventTypes::RELEASED_UI_LEFT);
        }
        else if (Keyboard::Down == event.key.code) {
          events.push_back(EventTypes::RELEASED_MOVE_DOWN);
          events.push_back(EventTypes::RELEASED_UI_DOWN);
        }
        else if (Keyboard::Right == event.key.code) {
          events.push_back(EventTypes::RELEASED_MOVE_RIGHT);
          events.push_back(EventTypes::RELEASED_UI_RIGHT);
        }
        else if (Keyboard::X == event.key.code) {
          events.push_back(EventTypes::RELEASED_CONFIRM);
          events.push_back(EventTypes::RELEASED_USE_CHIP);
        }
        else if (Keyboard::Z == event.key.code) {
          events.push_back(EventTypes::RELEASED_CANCEL);
          events.push_back(EventTypes::RELEASED_SHOOT);
        }
        else if (Keyboard::Space == event.key.code) {
          events.push_back(EventTypes::RELEASED_CUST_MENU);
          events.push_back(EventTypes::RELEASED_QUICK_OPT);
        }
        else if (Keyboard::P == event.key.code) {
          events.push_back(EventTypes::RELEASED_PAUSE);
        }
        else if (Keyboard::A == event.key.code) {
          events.push_back(EventTypes::RELEASED_CUST_MENU);
        }
        else if (Keyboard::S == event.key.code) {
          events.push_back(EventTypes::RELEASED_SPECIAL);
        }
        else if (Keyboard::D == event.key.code) {
          events.push_back(EventTypes::RELEASED_SCAN_LEFT);
        }
        else if (Keyboard::F == event.key.code) {
          events.push_back(EventTypes::RELEASED_SCAN_RIGHT);
        }
      }
    }
  } // end event poll

  // Check these every frame regardless of input state...
  lastAxisXPower = axisXPower;
  lastAxisYPower = axisYPower;

  axisXPower = 0.f;
  axisYPower = 0.f;

  if (sf::Joystick::isConnected(GAMEPAD_1)) {

    if (sf::Joystick::hasAxis(GAMEPAD_1, sf::Joystick::PovX)) {
      axisXPower = sf::Joystick::getAxisPosition(GAMEPAD_1, sf::Joystick::PovX);
    }

    if (sf::Joystick::hasAxis(GAMEPAD_1, sf::Joystick::PovY)) {
      axisYPower = sf::Joystick::getAxisPosition(GAMEPAD_1, sf::Joystick::PovY);
    }

    if (axisXPower <= -GAMEPAD_1_AXIS_SENSITIVITY) {
      lastButton = Gamepad::LEFT;

      auto action = settings.GetPairedActions((Gamepad)lastButton);

      for (auto a : action) {
        events.push_back({ a, InputState::PRESSED });
      }
    }
    
    if (axisXPower >= GAMEPAD_1_AXIS_SENSITIVITY) {
      lastButton = Gamepad::RIGHT;

      auto action = settings.GetPairedActions((Gamepad)lastButton);

      for (auto a : action) {
        events.push_back({ a, InputState::PRESSED });
      }
    }

    if (axisYPower >= GAMEPAD_1_AXIS_SENSITIVITY) {
      lastButton = Gamepad::UP;

      auto action = settings.GetPairedActions((Gamepad)lastButton);

      for (auto a : action) {
        events.push_back({ a, InputState::PRESSED });
      }
    }

    if (axisYPower <= -GAMEPAD_1_AXIS_SENSITIVITY) {
      lastButton = Gamepad::DOWN;

      auto action = settings.GetPairedActions((Gamepad)lastButton);

      for (auto a : action) {
        events.push_back({ a, InputState::PRESSED });
      }
    }

    if (axisXPower - lastAxisXPower != 0.f) {
      if (axisXPower - lastAxisXPower >= -GAMEPAD_1_AXIS_SENSITIVITY) {
        auto action = settings.GetPairedActions(Gamepad::LEFT);

        for (auto a : action) {
          events.push_back({ a, InputState::RELEASED });
        }
      }

      if (axisXPower - lastAxisXPower <= GAMEPAD_1_AXIS_SENSITIVITY) {
        auto action = settings.GetPairedActions(Gamepad::RIGHT);

        for (auto a : action) {
          events.push_back({ a, InputState::RELEASED });
        }
      }
    }

    if (axisYPower - lastAxisYPower != 0.f) {
      if (axisYPower - lastAxisYPower >= -GAMEPAD_1_AXIS_SENSITIVITY) {
        auto action = settings.GetPairedActions(Gamepad::DOWN);

        for (auto a : action) {
          events.push_back({ a, InputState::RELEASED });
        }
      }

      if (axisYPower - lastAxisYPower <= GAMEPAD_1_AXIS_SENSITIVITY) {
        auto action = settings.GetPairedActions(Gamepad::UP);

        for (auto a : action) {
          events.push_back({ a, InputState::RELEASED });
        }
      }
    }
  }

  // First, we must see if any held keys from the last frame are released this frame
  for (auto e : events) {
    if (e.state == InputState::RELEASED) {
      InputEvent find1 = { e.name, InputState::PRESSED };
      InputEvent find2 = { e.name, InputState::HELD };
      auto trunc = std::remove(eventsLastFrame.begin(), eventsLastFrame.end(), find1);
      eventsLastFrame.erase(trunc, eventsLastFrame.end());

      trunc = std::remove(eventsLastFrame.begin(), eventsLastFrame.end(), find2);
      eventsLastFrame.erase(trunc, eventsLastFrame.end());

      trunc = std::remove(events.begin(), events.end(), find1);
      events.erase(trunc, events.end());

      trunc = std::remove(events.begin(), events.end(), find2);
      events.erase(trunc, events.end());
    }
  }

  // Finally, merge the continuous held key events into the final event buffer
  for (auto e : eventsLastFrame) {
    InputEvent insert = EventTypes::NONE;
    InputEvent erase  = EventTypes::NONE;

      // Search for the held events to migrate into the current event frame
      // Prevent further events if a key is held
    if (e.state == HELD) {
      insert = e;
      erase = { e.name, PRESSED };
    }
   
    // Search for press keys that have been held and transform them
    if (e.state == PRESSED) {
      insert = { e.name, HELD };
      erase = e;
    }
   
    auto trunc = std::remove(events.begin(), events.end(), erase);
    events.erase(trunc, events.end());

    if (insert != EventTypes::NONE && std::find(events.begin(), events.end(), insert) == events.end()) {
      events.push_back(insert); // migrate this input
    }
  }
  
  /*
  // Uncomment for debugging
  for (auto e : events) {
    std::string state = "NONE";

    switch (e.state) {
    case InputState::HELD:
      state = "HELD";
      break;
    case InputState::RELEASED:
      state = "RELEASED";
      break;
    case InputState::PRESSED:
      state = "PRESSED";
      break;
    }
    Logger::Logf("input event %s, %s", e.name.c_str(), state.c_str());
  }
  */

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

const bool InputManager::IsJosytickAvailable() const
{
  return sf::Joystick::isConnected(GAMEPAD_1);
}

const bool InputManager::HasSystemCopyEvent() const
{
  return systemCopyEvent;
}

const bool InputManager::HasSystemPasteEvent() const
{
  return systemPasteEvent;
}

ConfigSettings InputManager::GetConfigSettings()
{
  return settings;
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

const std::string InputManager::GetInputBuffer()
{
  return inputBuffer;
}

void InputManager::HandleInputBuffer(sf::Event e) {
  if ((e.KeyPressed && e.key.code == sf::Keyboard::BackSpace) || (e.text.unicode == 8 && inputBuffer.size() != 0)) {
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
