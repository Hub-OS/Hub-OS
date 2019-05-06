#include <SFML/Window.hpp>
using sf::Event;
using sf::Keyboard;
#include "bnEngine.h"
#include "bnInputManager.h"
#include "bnDirection.h"

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
}


InputManager::~InputManager() {
  this->config = nullptr;
}

void InputManager::SupportChronoXGamepad(ChronoXConfigReader& config) {
  this->config = &config;
}

void InputManager::Update() {
  this->eventsLastFrame = this->events;
  this->events.clear();

  Event event;
  while (ENGINE.GetWindow()->pollEvent(event)) {
    if (event.type == Event::Closed) {
      ENGINE.GetWindow()->close();
    }

    if (event.type == sf::Event::TextEntered && this->captureInputBuffer) {
      this->HandleInputBuffer(event);
    }

    if (config && config->IsOK() && sf::Joystick::isConnected(GAMEPAD_1)) {
      for (unsigned int i = 0; i < sf::Joystick::getButtonCount(GAMEPAD_1); i++)
      {
        std::string action = "";

        if (sf::Joystick::isButtonPressed(GAMEPAD_1, i)) {
          action = config->GetPairedAction((ChronoXConfigReader::Gamepad)i);

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
          }
        } else {
          action = config->GetPairedAction((ChronoXConfigReader::Gamepad)i);

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
          events.push_back(PRESSED_A); // shoot 
        }
        else if (Keyboard::Z == event.key.code) {
          events.push_back(PRESSED_B); // use chip 
        }
        else if (Keyboard::Space == event.key.code) {
          events.push_back(PRESSED_START); // chip select
        }
        else if (Keyboard::P == event.key.code) {
          events.push_back(PRESSED_PAUSE);
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
        else if (Keyboard::Space == event.key.code) {
          events.push_back(RELEASED_A);
        }
        else if (Keyboard::RControl == event.key.code) {
          events.push_back(RELEASED_B);
        }
        else if (Keyboard::Return == event.key.code) {
          events.push_back(RELEASED_START);
        }
        else if (Keyboard::P == event.key.code) {
          events.push_back(RELEASED_PAUSE);
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
    };

    auto trunc = std::remove(events.begin(), events.end(), erase);
    events.erase(trunc, events.end());

    if (insert != InputEvent::NONE && std::find(events.begin(), events.end(), insert) == events.end()) {
      events.push_back(insert); // migrate this input
    }
  }

  eventsLastFrame.clear();

  // std::cout << "events size: " << events.size() << std::endl;

#if defined(__ANDROID__)
TouchArea::poll();
#endif

}

bool InputManager::Has(InputEvent _event) {
  return events.end() != find(events.begin(), events.end(), _event);
}

void InputManager::VirtualKeyEvent(InputEvent event) {
  events.push_back(event);
}

bool InputManager::Empty() {
  return events.empty();
}

bool InputManager::HasChronoXGamepadSupport()
{
  return config && (config->IsOK() && sf::Joystick::isConnected(GAMEPAD_1));
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
  if (e.KeyPressed == sf::Keyboard::BackSpace || (e.text.unicode == 8) && inputBuffer.size() != 0) {
    inputBuffer.pop_back();
  }
  else if (e.text.unicode < 128 && e.text.unicode != 8) {
    //std::cout << e.text.unicode << std::endl;
    inputBuffer.push_back((char)e.text.unicode);
  }
}

void InputManager::SetInputBuffer(std::string buff)
{
  inputBuffer = buff;
}
