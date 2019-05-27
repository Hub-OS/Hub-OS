#pragma once
#include <vector>
#include <map>
<<<<<<< HEAD
#include <SFML/Window/Event.hpp>

using std::map;
using std::vector;
=======
>>>>>>> b486e21e11627262088deae73097eaa7af56791c

#include "bnInputEvent.h"
#include "bnChronoXConfigReader.h"

using std::map;
using std::vector;

/**
 * @class InputManager
 * @author mav
 * @date 01/05/19
 * @brief Handles SFML event polling, checks for valid controller configurations, and manages event states
 */
class InputManager {
public:
  /**
   * @brief Get the singleton instance. If first call, initializes itself.
   * @return InputManager&
   */
  static InputManager& GetInstance();
  
  /**
   * @brief Frees config pointer
   */
  ~InputManager();
<<<<<<< HEAD
  void Update();
  bool Has(InputEvent _event);
  bool Empty();
=======
  
  /**
   * @brief Polls SFML's event queue used by windows and input devices
   * 
   * If controller is enabled, maps controller events to game input events
   * This is achieved through the configuration object that maps actions to paired
   * buttons.
   * 
   * Before this function ends, it will scan for any release events. If they are found,
   * they are cleared from the event list. If no release is found but a press is present,
   * the button state is transformed into a HELD state equivalent.
   */
  void Update();
  
  /**
   * @brief Queries if an input event has been fired
   * @param _event the event to look for.
   * @return true if present, false otherwise
   */
  bool Has(InputEvent _event);
  
  /**
   * @brief Checks if the input event list is empty
   * @return true if empty, false otherwise
   */
  bool Empty();
  
  /**
   * @brief Creates a reference to the config reader object
   * @param config
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  void SupportChronoXGamepad(ChronoXConfigReader& config);
  
  /**
   * @brief Returns true if the config reader is set, config file is valid, & a joystick is found
   * @return 
   */
  bool HasChronoXGamepadSupport();
  
  /**
   * @brief Begins capturing entered text instead of firing game input events
   * 
   * Used for name or text entry
   */
  void BeginCaptureInputBuffer();
  
  /**
   * @brief Ends text capture state
   * 
   * Used when leaving input fields
   */
  void EndCaptureInputBuffer();
  
  /**
   * @brief Returns the contents of the current captured text
   * @return const std::string
   */
  const std::string GetInputBuffer();
  
  /**
   * @brief Transforms SFML keycodes into ASCII char texts and stores into input buffer
   * @param e input keycode event
   */
  void HandleInputBuffer(sf::Event e);
  
  /**
   * @brief Overloads the input buffer
   * @param buff text to modify
   * 
   * Used when selecting existing input fields
   * After being set, the buffer can be deleted with backspace or modified by typing
   */
  void SetInputBuffer(std::string buff);

  void BeginCaptureInputBuffer();
  void EndCaptureInputBuffer();
  const std::string GetInputBuffer();
  void HandleInputBuffer(sf::Event e);
  void SetInputBuffer(std::string buff);


  void VirtualKeyEvent(InputEvent event);

private:
<<<<<<< HEAD
  bool captureInputBuffer;
  std::string inputBuffer;

  InputManager();
  vector<InputEvent> events;
  vector<InputEvent> eventsLastFrame;

  map<std::string, bool> gamepadPressed;
=======
  bool captureInputBuffer; /*!< Flags input buffer capture state */
  std::string inputBuffer; /*!< The internal input buffer data */

  /**
   * @brief sets all initial input events to false
   */
  InputManager();
  
  vector<InputEvent> events; /*!< Current event list */
  vector<InputEvent> eventsLastFrame; /*!< The even list prior to this update */

  map<std::string, bool> gamepadPressed; /*!< Maps controller events*/

  ChronoXConfigReader* config;   /*!< Support for ChronoX config.ini files */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c

};

/**
 * @brief macro to shorten manager calls
 */
#define INPUT InputManager::GetInstance()