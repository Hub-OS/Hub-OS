#pragma once
#include <string>
#include <map>
#include <list>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Joystick.hpp>

struct DiscordInfo {
  std::string user;
  std::string key;

};

/*! \brief easy to cast in with some special codes for joystick x/y axis */
enum Gamepad { BAD_CODE = -1, UP = 5555, LEFT = 5556, RIGHT = 5557, DOWN = 5558 };

struct ConfigSettings {
public:
  friend class ConfigReader;

  typedef std::multimap<sf::Keyboard::Key, std::string> KeyboardHash;
  typedef std::multimap<Gamepad, std::string> GamepadHash;

  /**
 * @brief If config file is ok
 * @return true if wellformed, false otherwise
 */
  const bool IsOK();

  /**
   * @brief Check if audio is on or off based on ini file
   * @return true if on, false otherwise
   */
  const bool IsAudioEnabled();

  const int GetMusicLevel();
  const int GetSFXLevel();
  void SetMusicLevel(int level);
  void SetSFXLevel(int level);
  /**
   * @brief For a keyboard event, return the action string
   * @param event sfml keyboard key
   * @return the mapped input event
   */
  const std::list<std::string> GetPairedActions(sf::Keyboard::Key event);

  /**
 * @brief For an action string, return the bound keyboard key
 * @param action name
 * @return the bound key
 */
  const sf::Keyboard::Key GetPairedInput(std::string action);

  /**
* @brief For an action string, return the bound gamepad event
* @param action name
* @return the bound gamepad event
*/
  const Gamepad GetPairedGamepadButton(std::string action);

  /**
   * @brief For a gamepad event, return the action string
   * @param Gamepad button
   * @return list of the mapped input events
   */
  const std::list<std::string> GetPairedActions(Gamepad event);

  ConfigSettings& operator=(ConfigSettings rhs);

  const DiscordInfo GetDiscordInfo() const;

  void SetKeyboardHash(const KeyboardHash key);
  void SetGamepadHash(const GamepadHash gamepad);

  ConfigSettings(const ConfigSettings& rhs);

  ConfigSettings();

private:
  // Config values
  // Map keys to actions
  KeyboardHash keyboard; /*!< Keyboard key to event */
  GamepadHash gamepad; /*!< Gamepad button to event */
  DiscordInfo discord;

  int musicLevel;
  int sfxLevel;

  // State flags
  bool isOK; /*!< true if the file was ok */
};