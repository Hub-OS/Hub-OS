#pragma once
#include "bnConfigReader.h"

void ConfigReader::Trim(std::string& line) {
  while (line.compare(0, 1, " ") == 0)
    line.erase(line.begin()); // remove leading whitespaces
  while (line.size() > 0 && line.compare(line.size() - 1, 1, " ") == 0)
    line.erase(line.end() - 1); // remove trailing whitespaces
}

std::string ConfigReader::ValueOf(std::string _key, std::string _line) {
  int keyIndex = (int)_line.find(_key);
  std::string s = _line.substr(keyIndex + _key.size() + 2);
  return s.substr(0, s.find("\""));
}

ConfigSettings::Gamepad ConfigReader::GetGamepadCode(int key) {

  return (ConfigSettings::Gamepad)key;
}

sf::Keyboard::Key ConfigReader::GetKeyCodeFromAscii(int ascii) {
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

const bool ConfigReader::Parse(std::string buffer) {
  return ParseDiscord(buffer);
}

const bool ConfigReader::ParseDiscord(std::string buffer) {
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
      settings.discord.user = value;
    }

    if (line.find("Key") != std::string::npos) {
      std::string value = ValueOf("Key", line);
      settings.discord.key = value;
    }

    // Read next line...
    buffer = buffer.substr(endline + 1);
  } while (endline > -1);

  return false;
}

const bool ConfigReader::ParseAudio(std::string buffer) {
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
      settings.isAudioEnabled = (enabledStr == "1");
    }

    // Read next line...
    buffer = buffer.substr(endline + 1);
  } while (endline > -1);

  return false;
}

const bool ConfigReader::ParseNet(std::string buffer) {
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

const bool ConfigReader::ParseVideo(std::string buffer) {
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

const bool ConfigReader::ParseKeyboard(std::string buffer) {
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
        settings.keyboard.insert(std::make_pair(GetKeyCodeFromAscii(std::atoi(value.c_str())), key));
      }
    }

    // Read next line...
    buffer = buffer.substr(endline + 1);
  } while (endline > -1);

  return false;
}


const bool ConfigReader::ParseGamepad(std::string buffer) {
  int endline = 0;

  do {
    endline = (int)buffer.find("\n");
    std::string line = buffer.substr(0, endline);

    Trim(line);

    for (auto key : EventTypes::KEYS) {
      if (line.find(key) != std::string::npos) {
        std::string value = ValueOf(key, line);
        settings.gamepad.insert(std::make_pair(GetGamepadCode(std::atoi(value.c_str())), key));
      }
    }

    // Read next line...
    buffer = buffer.substr(endline + 1);
  } while (endline > -1);

  // We've come to the end of the config file with all expected headers
  return true;
}

ConfigReader::ConfigReader(std::string filepath) {
  settings.isOK = Parse(FileUtil::Read(filepath));

  // We need SOME values
  // Provide default layout
  if (!settings.keyboard.size() || !settings.isOK) {
    settings.keyboard.insert(std::make_pair(sf::Keyboard::Left,  "Move Left"));
    settings.keyboard.insert(std::make_pair(sf::Keyboard::Up,    "Move Up"));
    settings.keyboard.insert(std::make_pair(sf::Keyboard::Right, "Move Right"));
    settings.keyboard.insert(std::make_pair(sf::Keyboard::Down,  "Move Down"));
    settings.keyboard.insert(std::make_pair(sf::Keyboard::Down, "Move Down"));
    settings.keyboard.insert(std::make_pair(sf::Keyboard::Z, "Shoot"));
    settings.keyboard.insert(std::make_pair(sf::Keyboard::X, "Use Chip"));
    settings.keyboard.insert(std::make_pair(sf::Keyboard::S, "Special"));
    settings.keyboard.insert(std::make_pair(sf::Keyboard::A, "Cust Menu"));
    settings.keyboard.insert(std::make_pair(sf::Keyboard::Return, "Pause"));
    settings.keyboard.insert(std::make_pair(sf::Keyboard::Up, "UI Up"));
    settings.keyboard.insert(std::make_pair(sf::Keyboard::Down, "UI Down"));
    settings.keyboard.insert(std::make_pair(sf::Keyboard::Left, "UI Left"));
    settings.keyboard.insert(std::make_pair(sf::Keyboard::Right, "UI Right"));
    settings.keyboard.insert(std::make_pair(sf::Keyboard::X, "Confirm"));
    settings.keyboard.insert(std::make_pair(sf::Keyboard::Z, "Cancel"));
    settings.keyboard.insert(std::make_pair(sf::Keyboard::Space, "Quick Opt"));
    settings.keyboard.insert(std::make_pair(sf::Keyboard::D, "Scan Left"));
    settings.keyboard.insert(std::make_pair(sf::Keyboard::F, "Scan Right"));
    
    settings.isAudioEnabled = true;

    settings.isOK = false; // This can be used as a flag that we're using default controls
  }
}

ConfigReader::~ConfigReader() { ; }

ConfigSettings ConfigReader::GetConfigSettings()
{
  return settings;
}
