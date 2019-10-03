/*! \brief All of the virtual input events to map to joysticks and keyboard commands */

#pragma once
enum InputState {
  PRESSED,
  HELD,
  RELEASED,
  NONE
};

struct InputEvent {
  std::string name;
  InputState state;

  const bool operator==(InputEvent& rhs) {
    return (rhs.name == this->name && rhs.state == this->state);
  }

  const bool operator!=(InputEvent& rhs) {
    return !(rhs == *this);
  }

  const bool operator==(const InputEvent& rhs) {
    return (rhs.name == this->name && rhs.state == this->state);
  }

  const bool operator!=(const InputEvent& rhs) {
    return !(rhs.name == this->name && rhs.state == this->state);
  }
};

namespace EventTypes {
  static const InputEvent NONE = { "", InputState::NONE };

  static const InputEvent PRESSED_MOVE_UP    = { "Move Up",    PRESSED };
  static const InputEvent PRESSED_MOVE_DOWN  = { "Move Down",  PRESSED };
  static const InputEvent PRESSED_MOVE_LEFT  = { "Move Left",  PRESSED };
  static const InputEvent PRESSED_MOVE_RIGHT = { "Move Right", PRESSED };
  static const InputEvent PRESSED_SHOOT      = { "Shoot",      PRESSED };
  static const InputEvent PRESSED_USE_CHIP   = { "Use Chip",   PRESSED };
  static const InputEvent PRESSED_SPECIAL    = { "Special",    PRESSED };
  static const InputEvent PRESSED_CUST_MENU  = { "Cust Menu",  PRESSED };
  static const InputEvent PRESSED_PAUSE      = { "Pause",      PRESSED };
  static const InputEvent PRESSED_UI_UP      = { "UI Up",      PRESSED };
  static const InputEvent PRESSED_UI_DOWN    = { "UI Down",    PRESSED };
  static const InputEvent PRESSED_UI_LEFT    = { "UI Left",    PRESSED };
  static const InputEvent PRESSED_UI_RIGHT   = { "UI Right",   PRESSED };
  static const InputEvent PRESSED_CONFIRM    = { "Confirm",    PRESSED };
  static const InputEvent PRESSED_CANCEL     = { "Cancel",     PRESSED };
  static const InputEvent PRESSED_QUICK_OPT  = { "Quick Opt",  PRESSED };
  static const InputEvent PRESSED_SCAN_LEFT  = { "Scan Left",  PRESSED };
  static const InputEvent PRESSED_SCAN_RIGHT = { "Scan Right", PRESSED };

  static const InputEvent RELEASED_MOVE_UP    = { "Move Up",    RELEASED };
  static const InputEvent RELEASED_MOVE_DOWN  = { "Move Down",  RELEASED };
  static const InputEvent RELEASED_MOVE_LEFT  = { "Move Left",  RELEASED };
  static const InputEvent RELEASED_MOVE_RIGHT = { "Move Right", RELEASED };
  static const InputEvent RELEASED_SHOOT      = { "Shoot",      RELEASED };
  static const InputEvent RELEASED_USE_CHIP   = { "Use Chip",   RELEASED };
  static const InputEvent RELEASED_SPECIAL    = { "Special",    RELEASED };
  static const InputEvent RELEASED_CUST_MENU  = { "Cust Menu",  RELEASED };
  static const InputEvent RELEASED_PAUSE      = { "Pause",      RELEASED };
  static const InputEvent RELEASED_UI_UP      = { "UI Up",      RELEASED };
  static const InputEvent RELEASED_UI_DOWN    = { "UI Down",    RELEASED };
  static const InputEvent RELEASED_UI_LEFT    = { "UI Left",    RELEASED };
  static const InputEvent RELEASED_UI_RIGHT   = { "UI Right",   RELEASED };
  static const InputEvent RELEASED_CONFIRM    = { "Confirm",    RELEASED };
  static const InputEvent RELEASED_CANCEL     = { "Cancel",     RELEASED };
  static const InputEvent RELEASED_QUICK_OPT  = { "Quick Opt",  RELEASED };
  static const InputEvent RELEASED_SCAN_LEFT  = { "Scan Left",  RELEASED };
  static const InputEvent RELEASED_SCAN_RIGHT = { "Scan Right", RELEASED };

  static const InputEvent HELD_MOVE_UP        = { "Move Up",    HELD };
  static const InputEvent HELD_MOVE_DOWN      = { "Move Down",  HELD };
  static const InputEvent HELD_MOVE_LEFT      = { "Move Left",  HELD };
  static const InputEvent HELD_MOVE_RIGHT     = { "Move Right", HELD };
  static const InputEvent HELD_SHOOT          = { "Shoot",      HELD };
  static const InputEvent HELD_USE_CHIP       = { "Use Chip",   HELD };
  static const InputEvent HELD_SPECIAL        = { "Special",    HELD };
  static const InputEvent HELD_CUST_MENU      = { "Cust Menu",  HELD };
  static const InputEvent HELD_PAUSE          = { "Pause",      HELD };
  static const InputEvent HELD_UI_UP          = { "UI Up",      HELD };
  static const InputEvent HELD_UI_DOWN        = { "UI Down",    HELD };
  static const InputEvent HELD_UI_LEFT        = { "UI Left",    HELD };
  static const InputEvent HELD_UI_RIGHT       = { "UI Right",   HELD };
  static const InputEvent HELD_CONFIRM        = { "Confirm",    HELD };
  static const InputEvent HELD_CANCEL         = { "Cancel",     HELD };
  static const InputEvent HELD_QUICK_OPT      = { "Quick Opt",  HELD };
  static const InputEvent HELD_SCAN_LEFT      = { "Scan Left",  HELD };
  static const InputEvent HELD_SCAN_RIGHT     = { "Scan Right", HELD };

  static const std::string KEYS[] = { "Move Up", "Move Down", "Move Left", "Move Right", "Shoot",
                                    "Use Chip", "Special", "Cust Menu", "Pause", "UI Up", "UI Left", "UI Right", 
                                    "Confirm", "Cancel", "Quick Opt" };
};