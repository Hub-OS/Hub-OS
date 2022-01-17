#pragma once
#include <filesystem>
#include <map>
#include <string_view>
#include "bnInputEvent.h"
#include "bnFileUtil.h"
#include "bnConfigSettings.h"
/*
Example file contents
---------------------

[Discord]
User="JohnDoe"
Key="ABCDE"
[Audio()]
Play="1"
[Video]
Filter="0"
Size="1"
[Keyboard]
Select="8"
Start="13"
R="83"
L="65"
B="88"
A="90"
Left="37"
Down="40"
Right="39"
Up="38"
[Gamepad]
Select="32777"
Start="32778"
R="32774"
L="32773"
B="32769"
A="32770"
Left="32783"
Down="32782"
Right="32784"
Up="32781"
*/

/**
 * @class ConfigReader
 * @author mav
 * @date 05/05/19
 * @brief Loads customized config .ini files
 */
class ConfigReader {
private:
  enum class Section {
    discord,
    audio,
    net,
    video,
    general,
    keyboard,
    gamepad,
    none,
  };

  ConfigSettings settings;
  std::filesystem::path filepath;
  bool isOK{ false };

  /**
   * @brief Aux function. Trim leading and trailing whitespaces
   * @param line string to modifiy
   */
  std::string_view Trim(std::string_view line);

  /**
   * @brief Aux function. Find the key stored in a line.
   * @param line expected format: `Key="value"`
   * @return Key
   */
  std::string_view ResolveKey(std::string_view line);

  /**
   * @brief Aux function. Find the value stored in a line.
   * @param line expected format: `Key="value"`
   * @return value
   */
  std::string_view ResolveValue(std::string_view line);

  /**
   * @brief Resolves a line (such as [General]) into a section.
   * @param line
   * @return value
   */
  Section ResolveSection(std::string_view line);

  /**
   * @brief Deprecated.
   * @param key
   * @return Gamepad button
   */
  Gamepad GetGamepadCode(int key);
  /**
   * @brief Transform ascii values to SFML events
   * @param ascii code
   * @return sf::Keyboard::Key
   */
  sf::Keyboard::Key GetKeyCodeFromAscii(int ascii);

  // Parsing

  /**
   * @brief Parse entire file contents
   * @param buffer file contents
   * @return true if entire file is good
   */
  bool Parse(std::string_view buffer);

  /**
   * @brief Parse a line containing a key and value in the format `Key="value"`
   * @param section
   * @param line
   * @return true if property is good, false if malformed
   */
  bool ParseProperty(Section section, std::string_view line);

  /**
   * @brief Parse a property from the Discord section
   * @param line
   * @return true if property is good, false if malformed
   */
  bool ParseDiscordProperty(std::string_view line);

  /**
   * @brief Parse a property from the Audio section
   * @param line
   * @return true if property is good, false if malformed
   */
  bool ParseAudioProperty(std::string_view line);

  /**
   * @brief Parse a property from the Net section
   * @param line
   * @return true if property is good, false if malformed
   */
  bool ParseNetProperty(std::string_view line);

  /**
   * @brief Parse a property from the Video section
   * @param line
   * @return true if property is good, false if malformed
   */
  bool ParseVideoProperty(std::string_view line);

  /**
   * @brief Parse a property from the General section
   * @param line
   * @return true if property is good, false if malformed
   */
  bool ParseGeneralProperty(std::string_view line);

  /**
   * @brief Parse a property from the Keyboard section
   * @param line
   * @return true if property is good, false if malformed
   */
  bool ParseKeyboardProperty(std::string_view line);

  /**
   * @brief Parse a property from the Gamepad section
   * @param line
   * @return true if property is good, false if malformed
   */
  bool ParseGamepadProperty(std::string_view line);

public:

  /**
   * @brief Parses config ini file at filepath
   * @param filepath path to ini file
   */
  ConfigReader(const std::filesystem::path& filepath);

  ~ConfigReader();

  ConfigSettings GetConfigSettings();
  const std::filesystem::path GetPath() const;
  bool IsOK();
};
