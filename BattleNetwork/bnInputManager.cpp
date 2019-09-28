#include <SFML/Window.hpp>
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

InputManager::InputManager() : config(nullptr) {

  if (sf::Joystick::isConnected(GAMEPAD_1)) {
    gamepadPressed["Start"] = false;
    gamepadPressed["Select"] = false;
    gamepadPressed["L"] = false;
    gamepadPressed["R"] = false;
    gamepadPressed["A"] = false;
    gamepadPressed["B"] = false;
    gamepadPressed["Left"] = false;
    gamepadPressed["Right"] = false;
    gamepadPressed["Up"] = false;
    gamepadPressed["Down"] = false;
  }

  lastkey = sf::Keyboard::Key::Unknown;
}


InputManager::~InputManager() {
  this->config = nullptr;
}

void InputManager::SupportConfigSettings(ConfigReader& config) {
  this->config = &config;
}

void InputManager::Update() {
  this->eventsLastFrame = this->events;
  this->events.clear();

  Event event;

  lastkey = sf::Keyboard::Key::Unknown;

  while (ENGINE.GetWindow()->pollEvent(event)) {
    if (event.type == Event::Closed) {
      this->onLoseFocus();
      ENGINE.GetWindow()->close();
    }

    if (event.type == Event::LostFocus) {
      this->onLoseFocus();
    }
    else if (event.type == Event::GainedFocus) {
      this->onRegainFocus();
    }
    else if (event.type == Event::Resized) {
      this->onResized(event.size.width, event.size.height);
    }

    if (event.type == sf::Event::TextEntered && this->captureInputBuffer) {
      this->HandleInputBuffer(event);
    }

    if(event.type == sf::Event::KeyPressed) {
      lastkey = event.key.code;
    }

    if (sf::Joystick::isConnected(GAMEPAD_1)) {
      for (unsigned int i = 0; i < sf::Joystick::getButtonCount(GAMEPAD_1); i++)
      {
        std::string action = "";

        if (sf::Joystick::isButtonPressed(GAMEPAD_1, i)) {
          action = config->GetPairedAction((ConfigReader::Gamepad)i);

          if (action == "") continue;

          if (!gamepadPressed[action]) {
            gamepadPressed[action] = true;

            // std::cout << "Button #" << i << " is pressed [Action: " << action << "]\n";

            if (action == "Select") {
              events.push_back(PRESSED_PAUSE);
            }
            else if (action == "Start") {
              events.push_back(PRESSED_START);
            }
            else if (action == "Left") {
              events.push_back(PRESSED_LEFT);
            }
            else if (action == "Right") {
              events.push_back(PRESSED_RIGHT);
            }
            else if (action == "Up") {
              events.push_back(PRESSED_UP);
            }
            else if (action == "Down") {
              events.push_back(PRESSED_DOWN);
            }
            else if (action == "A") {
              events.push_back(PRESSED_A);
            }
            else if (action == "B") {
              events.push_back(PRESSED_B);
            }
            else if (action == "LPAD") {
              events.push_back(PRESSED_LPAD);
            }
            else if (action == "RPAD") {
              events.push_back(PRESSED_RPAD);
            }
          }
        } else {
          action = config->GetPairedAction((ConfigReader::Gamepad)i);

          if (action == "") continue;

          if (gamepadPressed[action]) {
            gamepadPressed[action] = false;

            // std::cout << "Button #" << i << " is released [Action: " << action << "]\n";

            if (action == "Select") {
              events.push_back(RELEASED_PAUSE);
            }
            else if (action == "Start") {
              events.push_back(RELEASED_START);
            }
            else if (action == "Left") {
              events.push_back(RELEASED_LEFT);
            }
            else if (action == "Right") {
              events.push_back(RELEASED_RIGHT);
            }
            else if (action == "Up") {
              events.push_back(RELEASED_UP);
            }
            else if (action == "Down") {
              events.push_back(RELEASED_DOWN);
            }
            else if (action == "A") {
              events.push_back(RELEASED_A);
            }
            else if (action == "B") {
              events.push_back(RELEASED_B);
            }
            else if (action == "LPAD") {
              events.push_back(RELEASED_LPAD);
            }
            else if (action == "RPAD") {
              events.push_back(RELEASED_RPAD);
            }
          }
        }
      }
    } else if (Event::KeyPressed == event.type) {
      /* Gamepad not connected. Strictly use keyboard events. */
      std::string action = "";
      if (config && config->IsOK()) {
        action = config->GetPairedAction(event.key.code);

        if (action == "Select") {
          events.push_back(PRESSED_PAUSE);
        } else if (action == "Start") {
          events.push_back(PRESSED_START);
        } else if (action == "Left") {
          events.push_back(PRESSED_LEFT);
        }else if (action == "Right") {
          events.push_back(PRESSED_RIGHT);
        } else if (action == "Up") {
          events.push_back(PRESSED_UP);
        } else if (action == "Down") {
          events.push_back(PRESSED_DOWN);
        } else if (action == "A") {
          events.push_back(PRESSED_A);
        } else if (action == "B") {
          events.push_back(PRESSED_B);
        } else if (action == "LPAD") {
          events.push_back(PRESSED_LPAD);
        }
        else if (action == "RPAD") {
          events.push_back(PRESSED_RPAD);
        }
      } else {
        if (Keyboard::Up == event.key.code) {
          events.push_back(PRESSED_UP);
        }
        else if (Keyboard::Left == event.key.code) {
          events.push_back(PRESSED_LEFT);
        }
        else if (Keyboard::Down == event.key.code) {
          events.push_back(PRESSED_DOWN);
        }
        else if (Keyboard::Right == event.key.code) {
          events.push_back(PRESSED_RIGHT);
        }
        else if (Keyboard::X == event.key.code) {
          events.push_back(PRESSED_B); // use chip 
        }
        else if (Keyboard::Z == event.key.code) {
          events.push_back(PRESSED_A); // shoot
        }
        else if (Keyboard::Space == event.key.code) {
          events.push_back(PRESSED_START); 
        }
        else if (Keyboard::P == event.key.code) {
          events.push_back(PRESSED_PAUSE);
        }
        else if (Keyboard::A == event.key.code) {
          events.push_back(PRESSED_LPAD);
        }
        else if (Keyboard::S == event.key.code) {
          events.push_back(PRESSED_RPAD);
        }
      }
    } else if (Event::KeyReleased == event.type) {
      std::string action = "";
      if (config && config->IsOK()) {
        action = config->GetPairedAction(event.key.code);

        if (action == "Select") {
          events.push_back(RELEASED_PAUSE);
        }
        else if (action == "Start") {
          events.push_back(RELEASED_START);
        }
        else if (action == "Left") {
          events.push_back(RELEASED_LEFT);
        }
        else if (action == "Right") {
          events.push_back(RELEASED_RIGHT);
        }
        else if (action == "Up") {
          events.push_back(RELEASED_UP);
        }
        else if (action == "Down") {
          events.push_back(RELEASED_DOWN);
        }
        else if (action == "A") {
          events.push_back(RELEASED_A);
        }
        else if (action == "B") {
          events.push_back(RELEASED_B);
        }
        else if (action == "LPAD") {
          events.push_back(RELEASED_LPAD);
        }
        else if (action == "RPAD") {
          events.push_back(RELEASED_RPAD);
        }
      }
      else {
        if (Keyboard::Up == event.key.code) {
          events.push_back(RELEASED_UP);
        }
        else if (Keyboard::Left == event.key.code) {
          events.push_back(RELEASED_LEFT);
        }
        else if (Keyboard::Down == event.key.code) {
          events.push_back(RELEASED_DOWN);
        }
        else if (Keyboard::Right == event.key.code) {
          events.push_back(RELEASED_RIGHT);
        }
        else if (Keyboard::X == event.key.code) {
          events.push_back(RELEASED_B);
        }
        else if (Keyboard::Z == event.key.code) {
          events.push_back(RELEASED_A);
        }
        else if (Keyboard::Space == event.key.code) {
          events.push_back(RELEASED_START);
        }
        else if (Keyboard::P == event.key.code) {
          events.push_back(RELEASED_PAUSE);
        }
        else if (Keyboard::A == event.key.code) {
          events.push_back(RELEASED_LPAD);
        }
        else if (Keyboard::S == event.key.code) {
          events.push_back(RELEASED_RPAD);
        }
      }
    }
  } // end event poll
  // Check these every frame regardless of input state...

  float axisXPower = 0.f;
  float axisYPower = 0.f;

  if (sf::Joystick::isConnected(GAMEPAD_1)) {

    if (sf::Joystick::hasAxis(GAMEPAD_1, sf::Joystick::PovX)) {
      axisXPower = sf::Joystick::getAxisPosition(GAMEPAD_1, sf::Joystick::PovX);
    }

    if (sf::Joystick::hasAxis(GAMEPAD_1, sf::Joystick::PovY)) {
      axisYPower = sf::Joystick::getAxisPosition(GAMEPAD_1, sf::Joystick::PovY);
    }

    if (axisXPower <= -GAMEPAD_1_AXIS_SENSITIVITY) {
      events.push_back(PRESSED_LEFT);
      gamepadPressed["Left"] = true;
    }
    else if (gamepadPressed["Left"]) {
      events.push_back(RELEASED_LEFT);
      gamepadPressed["Left"] = false;
    }

    if (axisXPower >= GAMEPAD_1_AXIS_SENSITIVITY) {
      events.push_back(PRESSED_RIGHT);
      gamepadPressed["Right"] = true;
    }
    else if (gamepadPressed["Right"]) {
      events.push_back(RELEASED_RIGHT);
      gamepadPressed["Right"] = false;
    }

    if (axisYPower >= GAMEPAD_1_AXIS_SENSITIVITY) {
      events.push_back(PRESSED_UP);
      gamepadPressed["Up"] = true;
    }
    else if (gamepadPressed["Up"]) {
      events.push_back(RELEASED_UP);
      gamepadPressed["Up"] = false;
    }

    if (axisYPower <= -GAMEPAD_1_AXIS_SENSITIVITY) {
      events.push_back(PRESSED_DOWN);
      gamepadPressed["Down"] = true;
    }
    else if (gamepadPressed["Down"]) {
      events.push_back(RELEASED_DOWN);
      gamepadPressed["Down"] = false;
    }
  }

  // First, we must see if any held keys from the last frame are released this frame
  for (auto e : events) {
    InputEvent find1 = InputEvent::NONE;
    InputEvent find2 = InputEvent::NONE;

    switch (e) {
      // Searching for previous press events to transform into possible held events
    case InputEvent::RELEASED_A:
      find1 = InputEvent::HELD_A;
      find2 = InputEvent::PRESSED_A;
      break;
    case InputEvent::RELEASED_B:
      find1 = InputEvent::HELD_B;
      find2 = InputEvent::PRESSED_B;
      break;
    case InputEvent::RELEASED_PAUSE:
      find1 = InputEvent::HELD_PAUSE;
      find2 = InputEvent::PRESSED_PAUSE;
      break;
    case InputEvent::RELEASED_START:
      find1 = InputEvent::HELD_START;
      find2 = InputEvent::PRESSED_START;
      break;
    case InputEvent::RELEASED_LEFT:
      find1 = InputEvent::HELD_LEFT;
      find2 = InputEvent::PRESSED_LEFT;
      break;
    case InputEvent::RELEASED_RIGHT:
      find1 = InputEvent::HELD_RIGHT;
      find2 = InputEvent::PRESSED_RIGHT;
      break;
    case InputEvent::RELEASED_UP:
      find1 = InputEvent::HELD_UP;
      find2 = InputEvent::PRESSED_UP;
      break;
    case InputEvent::RELEASED_DOWN:
      find1 = InputEvent::HELD_DOWN;
      find2 = InputEvent::PRESSED_DOWN;
      break;
    case InputEvent::RELEASED_LPAD:
      find1 = InputEvent::HELD_LPAD;
      find2 = InputEvent::PRESSED_LPAD;
      break;
    case InputEvent::RELEASED_RPAD:
      find1 = InputEvent::HELD_RPAD;
      find2 = InputEvent::PRESSED_RPAD;
      break;
    }

    auto trunc = std::remove(eventsLastFrame.begin(), eventsLastFrame.end(), find1);
    eventsLastFrame.erase(trunc, eventsLastFrame.end());

    trunc = std::remove(eventsLastFrame.begin(), eventsLastFrame.end(), find2);
    eventsLastFrame.erase(trunc, eventsLastFrame.end());

    trunc = std::remove(events.begin(), events.end(), find1);
    events.erase(trunc, events.end());

    trunc = std::remove(events.begin(), events.end(), find2);
    events.erase(trunc, events.end());
  }

  // Finally, merge the continuous held key events into the final event buffer
  for (auto e : eventsLastFrame) {
    InputEvent insert = InputEvent::NONE;
    InputEvent erase  = InputEvent::NONE;

    switch (e) {
      // Search for the held events to migrate into the current event frame
      // Prevent further events if a key is held
    case InputEvent::HELD_A:
      insert = e;
      erase = PRESSED_A;
      break;
    case InputEvent::HELD_B:
      insert = e;
      erase = PRESSED_B;
      break;
    case InputEvent::HELD_PAUSE:
      insert = e;
      erase = PRESSED_PAUSE;
      break;
    case InputEvent::HELD_START:
      insert = e;
      erase = PRESSED_START;
      break;
    case InputEvent::HELD_LEFT:
      insert = e;
      erase = PRESSED_LEFT;
      break;
    case InputEvent::HELD_RIGHT:
      insert = e;
      erase = PRESSED_RIGHT;
      break;
    case InputEvent::HELD_UP:
      insert = e;
      erase = PRESSED_UP;
      break;
    case InputEvent::HELD_DOWN:
      insert = e;
      erase = PRESSED_DOWN;
      break;
    case InputEvent::HELD_LPAD:
      insert = e;
      erase = PRESSED_LPAD;
      break;
    case InputEvent::HELD_RPAD:
      insert = e;
      erase = PRESSED_RPAD;
      break;

      // Search for press keys that have been held and transform them
    case InputEvent::PRESSED_A:
      insert = InputEvent::HELD_A;
      erase = e;
      break;
    case InputEvent::PRESSED_B:
      insert = InputEvent::HELD_B;
      erase = e;
      break;
    case InputEvent::PRESSED_LEFT:
      insert = InputEvent::HELD_LEFT;
      erase = e;
      break;
    case InputEvent::PRESSED_RIGHT:
      insert = InputEvent::HELD_RIGHT;
      erase = e;
      break;
    case InputEvent::PRESSED_UP:
      insert = InputEvent::HELD_UP;
      erase = e;
      break;
    case InputEvent::PRESSED_DOWN:
      insert = InputEvent::HELD_DOWN;
      erase = e;
      break;
    case InputEvent::PRESSED_START:
      insert = InputEvent::HELD_START;
      erase = e;
      break;
    case InputEvent::PRESSED_PAUSE:
      insert = InputEvent::HELD_PAUSE;
      erase = e;
      break;
    case InputEvent::PRESSED_LPAD:
      insert = InputEvent::HELD_LPAD;
      erase = e;
      break;
    case InputEvent::PRESSED_RPAD:
      insert = InputEvent::HELD_RPAD;
      erase = e;
      break;
    };

    auto trunc = std::remove(events.begin(), events.end(), erase);
    events.erase(trunc, events.end());

    if (insert != InputEvent::NONE && std::find(events.begin(), events.end(), insert) == events.end()) {
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
      out = std::string(");"); return true;
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
      out = std::string("["); return true;
    case sf::Keyboard::Key::Slash:
      out = std::string("\\"); return true;
    case sf::Keyboard::Key::RBracket:
      out = std::string("]"); return true;
    case sf::Keyboard::Key::Quote:
      out = std::string("'"); return true;
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
  this->onRegainFocus = callback;
}

void InputManager::BindResizedEvent(std::function<void(int, int)> callback)
{
  this->onResized = callback;
}

void InputManager::BindLoseFocusEvent(std::function<void()> callback)
{
  this->onLoseFocus = callback;;
}

const bool InputManager::IsJosytickAvailable() const
{
  return sf::Joystick::isConnected(GAMEPAD_1);
}

bool InputManager::Empty() {
  return events.empty();
}

bool InputManager::IsConfigFileValid()
{
  return config && (config->IsOK());
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
  Logger::Log(std::string("e.KeyPressed: ") + std::to_string(e.KeyPressed) + " e.key.code: " + std::to_string(e.key.code));

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
