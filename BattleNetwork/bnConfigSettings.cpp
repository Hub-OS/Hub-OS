#include "bnConfigSettings.h"
#include "bnLogger.h"
#include "bnInputEvent.h"

/**
 * @brief If config file is ok
 * @return true if wellformed, false otherwise
 */
const bool ConfigSettings::IsOK() const { return isOK; }

/**
 * @brief Check if audio is on or off based on ini file
 * @return true if on, false otherwise
 */
const bool ConfigSettings::IsAudioEnabled() const { return (musicLevel || sfxLevel); }

const bool ConfigSettings::IsFullscreen() const
{
    return fullscreen;
}

const int ConfigSettings::GetMusicLevel() const { return musicLevel; }

const int ConfigSettings::GetSFXLevel() const { return sfxLevel; }

const bool ConfigSettings::IsKeyboardOK() const { 
    bool hasUp      = GetPairedInput(EventTypes::PRESSED_UI_UP.name) != sf::Keyboard::Unknown;
    bool hasDown    = GetPairedInput(EventTypes::PRESSED_UI_DOWN.name) != sf::Keyboard::Unknown;
    bool hasLeft    = GetPairedInput(EventTypes::PRESSED_UI_LEFT.name) != sf::Keyboard::Unknown;
    bool hasRight   = GetPairedInput(EventTypes::PRESSED_UI_RIGHT.name) != sf::Keyboard::Unknown;
    bool hasConfirm = GetPairedInput(EventTypes::PRESSED_CONFIRM.name) != sf::Keyboard::Unknown;

    return hasUp && hasDown && hasLeft && hasRight && hasConfirm;
}

void ConfigSettings::SetMusicLevel(int level) { musicLevel = level; }

void ConfigSettings::SetSFXLevel(int level) { sfxLevel = level; }

const std::list<std::string> ConfigSettings::GetPairedActions(const sf::Keyboard::Key& event) const {
    std::list<std::string> list;

    auto iter = keyboard.cbegin();

    while (iter != keyboard.end()) {
        if (iter->first == event) {
            list.push_back(iter->second);
        }
        iter++;
    }

    return list;
}

const sf::Keyboard::Key ConfigSettings::GetPairedInput(std::string action) const
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

const Gamepad ConfigSettings::GetPairedGamepadButton(std::string action) const
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

const std::list<std::string> ConfigSettings::GetPairedActions(const Gamepad& event) const {
    std::list<std::string> list;

    if (gamepad.size()) {
        auto iter = gamepad.cbegin();

        while (iter != gamepad.end()) {
            if (iter->first == event) {
                list.push_back(iter->second);
            }
            iter++;
        }
    }

    return list;
}

ConfigSettings & ConfigSettings::operator=(const ConfigSettings& rhs)
{
    this->discord = rhs.discord;
    this->webServer = rhs.webServer;
    this->gamepad = rhs.gamepad;
    this->musicLevel = rhs.musicLevel;
    this->sfxLevel = rhs.sfxLevel;
    this->isOK = rhs.isOK;
    this->keyboard = rhs.keyboard;
    this->fullscreen = rhs.fullscreen;
    return *this;
}

const DiscordInfo& ConfigSettings::GetDiscordInfo() const
{
    return this->discord;
}

const WebServerInfo& ConfigSettings::GetWebServerInfo() const
{
    return this->webServer;
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
    this->discord = rhs.discord;
    this->webServer = rhs.webServer;
    this->gamepad = rhs.gamepad;
    this->musicLevel = rhs.musicLevel;
    this->sfxLevel = rhs.sfxLevel;
    this->isOK = rhs.isOK;
    this->keyboard = rhs.keyboard;
    this->fullscreen = rhs.fullscreen;
}

ConfigSettings::ConfigSettings()
{
    isOK = false;
}
