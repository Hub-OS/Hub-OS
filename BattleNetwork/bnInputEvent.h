/*! \brief All of the virtual input events to map to joysticks and keyboard commands */
#pragma once

#include <string>

enum class InputState : unsigned char {
  none = 0,
  pressed,
  held,
  released
};

struct InputEvent {
  std::string name;
  InputState state{};

  const bool operator==(const InputEvent& rhs) {
    return (rhs.name == name && rhs.state == state);
  }

  const bool operator!=(const InputEvent& rhs) {
    return !(rhs.name == name && rhs.state == state);
  }
};

namespace InputEvents {
  static const InputEvent none{};

  static const InputEvent pressed_move_up    = { "Move Up",    InputState::pressed };
  static const InputEvent pressed_move_down  = { "Move Down",  InputState::pressed };
  static const InputEvent pressed_move_left  = { "Move Left",  InputState::pressed };
  static const InputEvent pressed_move_right = { "Move Right", InputState::pressed };
  static const InputEvent pressed_shoot      = { "Shoot",      InputState::pressed };
  static const InputEvent pressed_use_chip   = { "Use Card",   InputState::pressed };
  static const InputEvent pressed_special    = { "Special",    InputState::pressed };
  static const InputEvent pressed_cust_menu  = { "Cust Menu",  InputState::pressed };
  static const InputEvent pressed_pause      = { "Pause",      InputState::pressed };
  static const InputEvent pressed_ui_up      = { "UI Up",      InputState::pressed };
  static const InputEvent pressed_ui_down    = { "UI Down",    InputState::pressed };
  static const InputEvent pressed_ui_left    = { "UI Left",    InputState::pressed };
  static const InputEvent pressed_ui_right   = { "UI Right",   InputState::pressed };
  static const InputEvent pressed_confirm    = { "Confirm",    InputState::pressed };
  static const InputEvent pressed_cancel     = { "Cancel",     InputState::pressed };
  static const InputEvent pressed_option     = { "Option",     InputState::pressed };
  static const InputEvent pressed_scan_left  = { "Scan Left",  InputState::pressed };
  static const InputEvent pressed_scan_right = { "Scan Right", InputState::pressed };
  static const InputEvent pressed_run        = { "Run",        InputState::pressed };
  static const InputEvent pressed_interact   = { "Interact",   InputState::pressed };

  static const InputEvent released_move_up    = { "Move Up",    InputState::released };
  static const InputEvent released_move_down  = { "Move Down",  InputState::released };
  static const InputEvent released_move_left  = { "Move Left",  InputState::released };
  static const InputEvent released_move_right = { "Move Right", InputState::released };
  static const InputEvent released_shoot      = { "Shoot",      InputState::released };
  static const InputEvent released_use_chip   = { "Use Card",   InputState::released };
  static const InputEvent released_special    = { "Special",    InputState::released };
  static const InputEvent released_cust_menu  = { "Cust Menu",  InputState::released };
  static const InputEvent released_pause      = { "Pause",      InputState::released };
  static const InputEvent released_ui_up      = { "UI Up",      InputState::released };
  static const InputEvent released_ui_down    = { "UI Down",    InputState::released };
  static const InputEvent released_ui_left    = { "UI Left",    InputState::released };
  static const InputEvent released_ui_right   = { "UI Right",   InputState::released };
  static const InputEvent released_confirm    = { "Confirm",    InputState::released };
  static const InputEvent released_cancel     = { "Cancel",     InputState::released };
  static const InputEvent released_option     = { "Option",     InputState::released };
  static const InputEvent released_scan_left  = { "Scan Left",  InputState::released };
  static const InputEvent released_scan_right = { "Scan Right", InputState::released };
  static const InputEvent released_run        = { "Run",        InputState::released };
  static const InputEvent released_interact   = { "Interact",   InputState::released };

  static const InputEvent held_move_up        = { "Move Up",    InputState::held };
  static const InputEvent held_move_down      = { "Move Down",  InputState::held };
  static const InputEvent held_move_left      = { "Move Left",  InputState::held };
  static const InputEvent held_move_right     = { "Move Right", InputState::held };
  static const InputEvent held_shoot          = { "Shoot",      InputState::held };
  static const InputEvent held_use_chip       = { "Use Card",   InputState::held };
  static const InputEvent held_special        = { "Special",    InputState::held };
  static const InputEvent held_cust_menu      = { "Cust Menu",  InputState::held };
  static const InputEvent held_pause          = { "Pause",      InputState::held };
  static const InputEvent held_ui_up          = { "UI Up",      InputState::held };
  static const InputEvent held_ui_down        = { "UI Down",    InputState::held };
  static const InputEvent held_ui_left        = { "UI Left",    InputState::held };
  static const InputEvent held_ui_right       = { "UI Right",   InputState::held };
  static const InputEvent held_confirm        = { "Confirm",    InputState::held };
  static const InputEvent held_cancel         = { "Cancel",     InputState::held };
  static const InputEvent held_option         = { "Option",     InputState::held };
  static const InputEvent held_scan_left      = { "Scan Left",  InputState::held };
  static const InputEvent held_scan_right     = { "Scan Right", InputState::held };
  static const InputEvent held_run            = { "Run",        InputState::held };
  static const InputEvent held_interact       = { "Interact",   InputState::held };

  static const std::string KEYS[] = { "Move Up",  "Move Down", "Move Left", "Move Right", "Shoot",
                                      "Use Card", "Special",   "Cust Menu", "Pause",      "UI Up", 
                                      "UI Left",  "UI Right",  "UI Down",   "Confirm",    "Cancel",    
                                      "Option",   "Scan Left", "Scan Right","Run",        "Interact" };
};