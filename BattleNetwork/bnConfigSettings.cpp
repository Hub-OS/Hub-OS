#include "bnConfigSettings.h"
#include "bnLogger.h"

/**
 * @brief If config file is ok
 * @return true if wellformed, false otherwise
 */
const bool ConfigSettings::IsOK() { return isOK; }

/**
 * @brief Check if audio is on or off based on ini file
 * @return true if on, false otherwise
 */
const bool ConfigSettings::IsAudioEnabled() { return isAudioEnabled; }

/**
 * @brief For a keyboard event, return the action string
 * @param event sfml keyboard key
 * @return the mapped input event
 */
const std::string ConfigSettings::GetPairedAction(sf::Keyboard::Key event) {
  decltype(keyboard)::iterator iter = keyboard.find(event);


  if (iter != keyboard.end()) {
    return iter->second;
  }

  return "";
}

const sf::Keyboard::Key ConfigSettings::GetPairedInput(std::string action)
{
  auto iter = keyboard.begin();

  while (iter != keyboard.end()) {
    auto first = iter->second;
    auto second = action;

    std::transform(first.begin(), first.end(), first.begin(), ::toupper);
    std::transform(second.begin(), second.end(), second.begin(), ::toupper);

    if (first == second) {
      return iter->first;
    }
    iter++;
  }

  return sf::Keyboard::Unknown;
}

/**
 * @brief For a gamepad event, return the action string
 * @param Gamepad button
 * @return the mapped input event
 */
const std::string ConfigSettings::GetPairedAction(ConfigSettings::Gamepad event) {
  if (gamepad.size()) {
    decltype(gamepad)::iterator iter = gamepad.find(event);

    if (iter != gamepad.end()) {
      return iter->second;
    }
  }

  return "";
}

ConfigSettings & ConfigSettings::operator=(ConfigSettings rhs)
{
  this->discord.key = rhs.discord.key;
  this->discord.user = rhs.discord.user;
  this->gamepad = rhs.gamepad;
  this->isAudioEnabled = rhs.isAudioEnabled;
  this->isOK = rhs.isOK;
  this->keyboard = rhs.keyboard;
  return *this;
}

ConfigSettings::ConfigSettings(const ConfigSettings & rhs)
{
  this->discord.key = rhs.discord.key;
  this->discord.user = rhs.discord.user;
  this->gamepad = rhs.gamepad;
  this->isAudioEnabled = rhs.isAudioEnabled;
  this->isOK = rhs.isOK;
  this->keyboard = rhs.keyboard;
}

ConfigSettings::ConfigSettings()
{
  isOK = false;
}
