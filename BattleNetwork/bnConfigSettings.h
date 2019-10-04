#pragma once
#include <string>
#include <map>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Joystick.hpp>

struct ConfigSettings {
public:
  friend class ConfigReader;

  enum Gamepad { };

  struct DiscordInfo {
    std::string user;
    std::string key;
  };

  /**
 * @brief If config file is ok
 * @return true if wellformed, false otherwise
 */
  const bool ConfigSettings::IsOK();

  /**
   * @brief Check if audio is on or off based on ini file
   * @return true if on, false otherwise
   */
  const bool ConfigSettings::IsAudioEnabled();

  /**
   * @brief For a keyboard event, return the action string
   * @param event sfml keyboard key
   * @return the mapped input event
   */
  const std::string ConfigSettings::GetPairedAction(sf::Keyboard::Key event);

  /**
 * @brief For an action string, return the bound keyboard key
 * @param action name
 * @return the bound key
 */
  const sf::Keyboard::Key ConfigSettings::GetPairedInput(std::string action);

  /**
   * @brief For a gamepad event, return the action string
   * @param Gamepad button
   * @return the mapped input event
   */
  const std::string ConfigSettings::GetPairedAction(ConfigSettings::Gamepad event);

  ConfigSettings& operator=(ConfigSettings rhs);

  ConfigSettings(const ConfigSettings& rhs);

  ConfigSettings();

private:
  // Config values
  // Map keys to actions
  std::multimap<sf::Keyboard::Key, std::string> keyboard; /*!< Keyboard key to event */
  std::multimap<Gamepad, std::string> gamepad; /*!< Gamepad button to event */
  DiscordInfo discord;

  bool isAudioEnabled; /*!< Mute audio if flagged */

  // State flags
  bool isOK; /*!< true if the file was ok */
};