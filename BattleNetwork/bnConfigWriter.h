#pragma once
#include "bnConfigSettings.h"

#include <filesystem>

/*
Example file contents
---------------------

[Discord]
User="JohnDoe"
Key="ABCDE"
[Audio()]
Play="1"
[Net]
WebServer="battlenetwork.io"
Version="1"
Port="3030"
Username="Deltron"
Password="Mastermind"
[Video]
Fullscreen="0"
[Keyboard]
Move Up="8"
Move Left="13"
Move Right="83"
Move Down="65"
Use Card="88"
Special="90"
Shoot="37"
Cust="40"
UI Left="39"
UI Right="38"
UI Up="55"
UI Down="65"
[Gamepad]
Move Up="8"
Move Left="13"
Move Right="83"
Move Down="65"
Use Card="88"
Special="90"
Shoot="37"
Cust="40"
UI Left="39"
UI Right="38"
UI Up="55"
UI Down="65"
*/

class ConfigWriter {
  ConfigSettings& settings;
public:
  ConfigWriter(ConfigSettings& settings);
  ~ConfigWriter();

  void Write(const std::filesystem::path& path);

  const std::string quote(const std::string& str);

private:
  /**
   * @brief Transform Gamepad codes to ascii
   * @param Gamepad event code
   * @return int ascii
   */
  int GetAsciiFromGamepad(Gamepad code);
  /**
   * @brief Transform SFML events to ascii
   * @param sf::Keyboard::Key
   * @return int ascii
   */
  int GetAsciiFromKeyboard(sf::Keyboard::Key key);
};
