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
const bool ConfigSettings::IsAudioEnabled() { return (musicLevel || sfxLevel); }

const int ConfigSettings::GetMusicLevel()
{
  return musicLevel;
}

const int ConfigSettings::GetSFXLevel()
{
  return sfxLevel;
}

void ConfigSettings::SetMusicLevel(int level)
{
  musicLevel = level;
}

void ConfigSettings::SetSFXLevel(int level)
{
  sfxLevel = level;
}

const std::list<std::string> ConfigSettings::GetPairedActions(sf::Keyboard::Key event) {
  std::list<std::string> list;

  decltype(keyboard)::iterator iter = keyboard.begin();

  while (iter != keyboard.end()) {
    if (iter->first == event) {
      list.push_back(iter->second);
    }
    iter++;
  }

  return list;
}

const sf::Keyboard::Key ConfigSettings::GetPairedInput(std::string action)
{
  auto iter = keyboard.begin();

  while (iter != keyboard.end()) {
    auto first = iter->second;

    std::transform(first.begin(), first.end(), first.begin(), ::toupper);
    std::transform(action.begin(), action.end(), action.begin(), ::toupper);

    if (first == action) {
      return iter->first;
    }
    iter++;
  }

  return sf::Keyboard::Unknown;
}

const Gamepad ConfigSettings::GetPairedGamepadButton(std::string action)
{
  auto iter = gamepad.begin();

  while (iter != gamepad.end()) {
    auto first = iter->second;

    std::transform(first.begin(), first.end(), first.begin(), ::toupper);
    std::transform(action.begin(), action.end(), action.begin(), ::toupper);

    if (first == action) {
      return iter->first;
    }
    iter++;
  }

  return (Gamepad) - 1;
}

const std::list<std::string> ConfigSettings::GetPairedActions(Gamepad event) {
  std::list<std::string> list;

  if (gamepad.size()) {
    decltype(gamepad)::iterator iter = gamepad.begin();

    while (iter != gamepad.end()) {
      if (iter->first == event) {
        list.push_back(iter->second);
      }
      iter++;
    }
  }

  return list;
}

ConfigSettings & ConfigSettings::operator=(ConfigSettings rhs)
{
  this->discord.key = rhs.discord.key;
  this->discord.user = rhs.discord.user;
  this->gamepad = rhs.gamepad;
  this->musicLevel = rhs.musicLevel;
  this->sfxLevel = rhs.sfxLevel;
  this->isOK = rhs.isOK;
  this->keyboard = rhs.keyboard;
  return *this;
}

const DiscordInfo ConfigSettings::GetDiscordInfo() const
{
  return this->discord;
}

void ConfigSettings::SetKeyboardHash(const KeyboardHash key)
{
  keyboard = key;
}

void ConfigSettings::SetGamepadHash(const GamepadHash gamepad)
{
  this->gamepad = gamepad;
}

ConfigSettings::ConfigSettings(const ConfigSettings & rhs)
{
  this->discord.key = rhs.discord.key;
  this->discord.user = rhs.discord.user;
  this->gamepad = rhs.gamepad;
  this->musicLevel = rhs.musicLevel;
  this->sfxLevel = rhs.sfxLevel;
  this->isOK = rhs.isOK;
  this->keyboard = rhs.keyboard;
}

ConfigSettings::ConfigSettings()
{
  isOK = false;
}
