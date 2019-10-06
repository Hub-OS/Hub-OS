#pragma once
#include "bnConfigReader.h"
#include "bnInputManager.h"

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

Gamepad ConfigReader::GetGamepadCode(int key) {

  return (Gamepad)key;
}

sf::Keyboard::Key ConfigReader::GetKeyCodeFromAscii(int ascii) {
  return (sf::Keyboard::Key)ascii;
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
    else if (line.find("Music") != std::string::npos) {
      std::string enabledStr = ValueOf("Music", line);
      settings.musicLevel = std::atoi(enabledStr.c_str());
    }
    else if (line.find("SFX") != std::string::npos) {
      std::string enabledStr = ValueOf("SFX", line);
      settings.sfxLevel = std::atoi(enabledStr.c_str());
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
        auto code = GetKeyCodeFromAscii(std::atoi(value.c_str()));

        settings.keyboard.insert(std::make_pair(code, key));
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
    Logger::Log("config settings was not OK. Switching to default key layout.");
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
    
    settings.sfxLevel = settings.musicLevel = 3;

    settings.isOK = false; // This can be used as a flag that we're using default controls
  }
  else {
    Logger::Log("config settings is OK");

    /*Logger::Log("settings was: ");
    for (auto p : settings.keyboard) {
      std::string first;
      INPUT.ConvertKeyToString(p.first, first);
      Logger::Logf("key %s is %s", first.c_str(), p.second.c_str());
    }*/
  }
}

ConfigReader::~ConfigReader() { ; }

ConfigSettings ConfigReader::GetConfigSettings()
{
  return settings;
}
