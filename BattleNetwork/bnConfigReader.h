#pragma once
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Joystick.hpp>
#include <map>
#include "bnInputEvent.h"
#include "bnFileUtil.h"

/*
Example file contents
---------------------

[Discord]
RPC="1"
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
public:
  enum Gamepad { };

  struct DiscordInfo {
    std::string user;
    std::string key;
  };

private:
  // Config values
  // Map keys to actions
  std::map<sf::Keyboard::Key, std::string> keyboard; /*!< Keyboard key to event */
  std::map<Gamepad          , std::string> gamepad; /*!< Gamepad button to event */
  DiscordInfo discord;

  bool isAudioEnabled; /*!< Mute audio if flagged */

  // State flags
  bool isOK; /*!< true if the file was ok */

   /**
   * @brief Writes the config settings to disc
   * @param path string of output file path
   */
  void WriteToFile(std::string path) {
 
  }

  /**
   * @brief Aux function. Trim leading and trailing whitespaces
   * @param line string to modifiy
   */
  void Trim(std::string& line) {
    while (line.compare(0, 1, " ") == 0)
      line.erase(line.begin()); // remove leading whitespaces
    while (line.size() > 0 && line.compare(line.size() - 1, 1, " ") == 0)
      line.erase(line.end() - 1); // remove trailing whitespaces
  }

  /**
   * @brief Aux function. Given a key in a line, find the value.
   * @param _key
   * @param _line
   * @return value
   */
  std::string ValueOf(std::string _key, std::string _line) {
    int keyIndex = (int)_line.find(_key);
    std::string s = _line.substr(keyIndex + _key.size() + 2);
    return s.substr(0, s.find("\""));
  }

  /**
   * @brief Deprecated. 
   * @param key
   * @return Gamepad button
   */
  Gamepad GetGamepadCode(int key) {
    
    return (Gamepad)key;
  }

  /**
   * @brief Transform ascii values to SFML events
   * @param ascii code
   * @return sf::Keyboard::Key
   */
  sf::Keyboard::Key GetKeyCodeFromAscii(int ascii) {
    switch (ascii) {
    case 49:
      return sf::Keyboard::Key::Num1;
    case 50:
      return sf::Keyboard::Key::Num2;
    case 51:
      return sf::Keyboard::Key::Num3;
    case 52:
      return sf::Keyboard::Key::Num4;
    case 53:
      return sf::Keyboard::Key::Num5;
    case 54:
      return sf::Keyboard::Key::Num6;
    case 55:
      return sf::Keyboard::Key::Num7;
    case 56:
      return sf::Keyboard::Key::Num8;
    case 57:
      return sf::Keyboard::Key::Num9;
    case 58:
      return sf::Keyboard::Key::Num0;
    case 13:
      return sf::Keyboard::Key::Return;
    case 8: 
      return sf::Keyboard::Key::BackSpace;
    case 32:
      return sf::Keyboard::Key::Space;
    case 37:
      return sf::Keyboard::Key::Left;
    case 40:
      return sf::Keyboard::Key::Down;
    case 39:
      return sf::Keyboard::Key::Right;
    case 38:
      return sf::Keyboard::Key::Up;
    case 46:
      return sf::Keyboard::Key::Delete;
    case 65:
      return sf::Keyboard::Key::A;
    case 66:
      return sf::Keyboard::Key::B;
    case 67:
      return sf::Keyboard::Key::C;
    case 68:
      return sf::Keyboard::Key::D;
    case 69:
      return sf::Keyboard::Key::E;
    case 70:
      return sf::Keyboard::Key::F;
    case 71:
      return sf::Keyboard::Key::G;
    case 72:
      return sf::Keyboard::Key::H;
    case 73:
      return sf::Keyboard::Key::I;
    case 74:
      return sf::Keyboard::Key::J;
    case 75:
      return sf::Keyboard::Key::K;
    case 76:
      return sf::Keyboard::Key::L;
    case 77:
      return sf::Keyboard::Key::M;
    case 78:
      return sf::Keyboard::Key::N;
    case 79:
      return sf::Keyboard::Key::O;
    case 80:
      return sf::Keyboard::Key::P;
    case 81:
      return sf::Keyboard::Key::Q;
    case 82:
      return sf::Keyboard::Key::R;
    case 83:
      return sf::Keyboard::Key::S;
    case 84:
      return sf::Keyboard::Key::T;
    case 85:
      return sf::Keyboard::Key::U;
    case 86:
      return sf::Keyboard::Key::V;
    case 87:
      return sf::Keyboard::Key::W;
    case 88:
      return sf::Keyboard::Key::X;
    case 89:
      return sf::Keyboard::Key::Y;
    case 90:
      return sf::Keyboard::Key::Z;
    case 112:
      return sf::Keyboard::Key::Tilde;
    case 16:
      return sf::Keyboard::Key::Tab;
    case 162:
      return sf::Keyboard::Key::LControl;
    case 164:
      return sf::Keyboard::Key::LAlt;
    case 165:
      return sf::Keyboard::Key::RAlt;
    case 163:
      return sf::Keyboard::Key::RControl;
    case 186:
      return sf::Keyboard::Key::SemiColon;
    case 187:
      return sf::Keyboard::Key::Equal;
    case 188:
      return sf::Keyboard::Key::Comma;
    case 189:
      return sf::Keyboard::Key::Dash;
    case 190:
      return sf::Keyboard::Key::Period;
    case 191:
      return sf::Keyboard::Key::Divide;
    case 219:
      return sf::Keyboard::Key::LBracket;
    case 220:
      return sf::Keyboard::Key::Slash;
    case 221:
      return sf::Keyboard::Key::RBracket;
    case 222:
      return sf::Keyboard::Key::Quote;

    }

    return sf::Keyboard::Key::Unknown;
  }

  // Parsing
  
  /**
   * @brief Parse entire file contents
   * @param buffer file contents
   * @return true if entire file is good
   */
  const bool Parse(std::string buffer) {
    return ParseDiscord(buffer);
  }

  /**
   * @brief File begins with [Discord] and settings
   * @param buffer file contents
   * @return true if rest of contents are good, false if malformed
   * 
   * Expects [Audio] to be next
   */
  const bool ParseDiscord(std::string buffer) {
    int endline = 0;

    do {
      endline = (int)buffer.find("\n");
      std::string line = buffer.substr(0, endline);

      Trim(line);

      if (line.find("[Audio]") != std::string::npos) {
        return ParseAudio(buffer);
      }

      if (line.find("User") != std::string::npos) {
        std::string value = ValueOf("User", line);
        discord.user = value;
      }

      if (line.find("Key") != std::string::npos) {
        std::string value = ValueOf("Key", line);
        discord.key = value;
      }

      // Read next line...
      buffer = buffer.substr(endline + 1);
    } while (endline > -1);

    return false;
  }

  /**
   * @brief Parse [Audio] 
   * @param buffer file contents
   * @return true if file is good, false if malformed
   * 
   * expects [Net] to be next
   */
  const bool ParseAudio(std::string buffer) {
    int endline = 0;

    do {
      endline = (int)buffer.find("\n");
      std::string line = buffer.substr(0, endline);

      Trim(line);

      if (line.find("[Net]") != std::string::npos) {
        return ParseNet(buffer);
      }
      else if (line.find("Play") != std::string::npos) {
        std::string enabledStr = ValueOf("Play", line);
        isAudioEnabled = (enabledStr == "1");
      }

      // Read next line...
      buffer = buffer.substr(endline + 1);
    } while (endline > -1);

    return false;
  }

  /**
   * @brief Parse [Net] and settings
   * @param buffer file contents
   * @return true if file is good, false if malformed
   * 
   * Expects [Video] to be next
   */
  const bool ParseNet(std::string buffer) {
    int endline = 0;

    do {
      endline = (int)buffer.find("\n");
      std::string line = buffer.substr(0, endline);

      Trim(line);

      if (line.find("[Video]") != std::string::npos) {
        return ParseVideo(buffer);
      }

      // NOTE: networking will not be a feature for some time...

      // Read next line...
      buffer = buffer.substr(endline + 1);
    } while (endline > -1);

    return false;
  }

  /**
   * @brief Parses [Video] and settings
   * @param buffer file contents
   * @return true if good, false if malformed
   * 
   * expects [Keyboard] to be next
   */
  const bool ParseVideo(std::string buffer) {
    int endline = 0;

    do {
      endline = (int)buffer.find("\n");
      std::string line = buffer.substr(0, endline);

      Trim(line);

      if (line.find("[Keyboard]") != std::string::npos) {
        return ParseKeyboard(buffer);
      }

      // TODO: handle video settings

      // Read next line...
      buffer = buffer.substr(endline + 1);
    } while (endline > -1);

    return false;
  }

  /**
   * @brief Parse [Keyboard] and settings
   * @param buffer file contents
   * @return true if the file is ok, false otherwise
   * 
   * expects [Gamepad] next
   */
  const bool ParseKeyboard(std::string buffer) {
    int endline = 0;

    do {
      endline = (int)buffer.find("\n");
      std::string line = buffer.substr(0, endline);

      Trim(line);

      if (line.find("[Gamepad]") != std::string::npos) {
        return ParseGamepad(buffer);
      }


      for (auto key : EventTypes::KEYS) {
        if (line.find(key) != std::string::npos) {
          std::string value = ValueOf(key, line);
          keyboard.insert(std::make_pair(GetKeyCodeFromAscii(std::atoi(value.c_str())), key));
        }
      }

      // Read next line...
      buffer = buffer.substr(endline + 1);
    } while (endline > -1);

    return false;
  }

  /**
   * @brief Parses [Gamepad] and settings
   * @param buffer file contents
   * @return true and denotes end of file
   */
  const bool ParseGamepad(std::string buffer) {
    int endline = 0;

    do {
      endline = (int)buffer.find("\n");
      std::string line = buffer.substr(0, endline);

      Trim(line);

      for (auto key : EventTypes::KEYS) {
        if (line.find(key) != std::string::npos) {
          std::string value = ValueOf(key, line);
          gamepad.insert(std::make_pair(GetGamepadCode(std::atoi(value.c_str())), key));
        }
      }

      // Read next line...
      buffer = buffer.substr(endline + 1);
    } while (endline > -1);

    // We've come to the end of the config file with all expected headers
    return true;
  }

public:

  /**
   * @brief Parses config ini file at filepath
   * @param filepath path to ini file
   */
  ConfigReader(std::string filepath) : isOK(false) {
    isOK = Parse(FileUtil::Read(filepath));
  }

  /**
   * @brief If config file is ok
   * @return true if wellformed, false otherwise
   */
  const bool IsOK() { return isOK; }

  /**
   * @brief Check if audio is on or off based on ini file
   * @return true if on, false otherwise
   */
  const bool IsAudioEnabled() { return isAudioEnabled;  }

  /**
   * @brief For a keyboard event, return the action string
   * @param event sfml keyboard key
   * @return the mapped input event
   */
  const std::string GetPairedAction(sf::Keyboard::Key event) {
    std::map<sf::Keyboard::Key, std::string>::iterator iter = keyboard.find(event);


    if (iter != keyboard.end()) {
      return iter->second;
    }

    return "";
  }

  /**
   * @brief For a gamepad event, return the action string
   * @param Gamepad button
   * @return the mapped input event
   */
  const std::string GetPairedAction(Gamepad event) {
    std::map<Gamepad, std::string>::iterator iter = gamepad.find(event);

    if (iter != gamepad.end()) {
      return iter->second;
    }

    return "";
  }

  ~ConfigReader() { ; }
};