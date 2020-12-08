#pragma once
#include <memory>
#include <assert.h>

class InputManager;

class InputHandle {
  friend class Game;

private:
  static InputManager* input;

public:
  InputManager& Input() { 
    assert(input != nullptr && "input manager was nullptr!");  
    return *input; 
  }

  // const-qualified functions

  InputManager& Input() const {
    assert(input != nullptr && "input manager was nullptr!");
    return *input;
  }
};

InputManager* InputHandle::input = nullptr;