#pragma once
#include <map>
#include "bnInputEvent.h"
#include "bnFileUtil.h"
#include "bnConfigSettings.h"
/*
Example file contents
---------------------

[Discord]
User="JohnDoe"
Key="ABCDE"
[Audio]
Play="1"
[Net]
uPNP="0"
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
  ConfigSettings settings;

  /**
   * @brief Aux function. Trim leading and trailing whitespaces
   * @param line string to modifiy
   */
  void Trim(std::string& line);

  /**
   * @brief Aux function. Given a key in a line, find the value.
   * @param _key
   * @param _line
   * @return value
   */
  std::string ValueOf(std::string _key, std::string _line);

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
  const bool Parse(std::string buffer);

  /**
   * @brief File begins with [Discord] and settings
   * @param buffer file contents
   * @return true if rest of contents are good, false if malformed
   * 
   * Expects [Audio] to be next
   */
  const bool ParseDiscord(std::string buffer);

  /**
   * @brief Parse [Audio] 
   * @param buffer file contents
   * @return true if file is good, false if malformed
   * 
   * expects [Net] to be next
   */
  const bool ParseAudio(std::string buffer);

  /**
   * @brief Parse [Net] and settings
   * @param buffer file contents
   * @return true if file is good, false if malformed
   * 
   * Expects [Video] to be next
   */
  const bool ParseNet(std::string buffer);

  /**
   * @brief Parses [Video] and settings
   * @param buffer file contents
   * @return true if good, false if malformed
   * 
   * expects [Keyboard] to be next
   */
  const bool ParseVideo(std::string buffer);

  /**
   * @brief Parse [Keyboard] and settings
   * @param buffer file contents
   * @return true if the file is ok, false otherwise
   * 
   * expects [Gamepad] next
   */
  const bool ParseKeyboard(std::string buffer);

  /**
   * @brief Parses [Gamepad] and settings
   * @param buffer file contents
   * @return true and denotes end of file
   */
  const bool ParseGamepad(std::string buffer);

public:

  /**
   * @brief Parses config ini file at filepath
   * @param filepath path to ini file
   */
  ConfigReader(std::string filepath);

  ~ConfigReader();

  ConfigSettings GetConfigSettings();
};