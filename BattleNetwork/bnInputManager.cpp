#include <SFML/Window.hpp>
using sf::Event;
using sf::Keyboard;
#include "bnEngine.h"
#include "bnInputManager.h"
#include "bnDirection.h"

// #include <iostream>

#define GAMEPAD_1 0
#define GAMEPAD_1_AXIS_SENSITIVITY 30.f

InputManager& InputManager::GetInstance() {
  static InputManager instance;
  return instance;
}

InputManager::InputManager()
  : events(vector<InputEvent>()),
    config(nullptr) {

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

void InputManager::update() {
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
        else if (Keyboard::Space == event.key.code) {
          events.push_back(PRESSED_A); // shoot 
        }
        else if (Keyboard::RControl == event.key.code) {
          events.push_back(PRESSED_B); // use chip 
        }
        else if (Keyboard::Return == event.key.code) {
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
}

bool InputManager::has(InputEvent _event) {
  return events.end() != find(events.begin(), events.end(), _event);
}

bool InputManager::empty() {
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
