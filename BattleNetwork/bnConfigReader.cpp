#include "bnConfigReader.h"
#include "bnInputManager.h"

namespace {
  const auto LINE_BREAKS = { '\r', '\n' };
}

std::string_view ConfigReader::Trim(std::string_view line) {
  while (line.compare(0, 1, " ") == 0)
    line = line.substr(1); // remove leading whitespaces
  while (line.size() > 0 && line.compare(line.size() - 1, 1, " ") == 0)
    line = line.substr(0, line.size() - 1); // remove trailing whitespaces
  return line;
}

std::string_view ConfigReader::ResolveKey(std::string_view line) {
  auto end = line.find('=');

  if (end == std::string_view::npos) {
    return line.substr(0, 0);
  }

  return Trim(line.substr(0, end));
}

std::string_view ConfigReader::ResolveValue(std::string_view line) {
  auto start = line.find('"') + 1;
  auto end = line.find('"', start);

  if (end == std::string_view::npos || start > end) {
    return line.substr(0, 0);
  }

  return line.substr(start, end - start);
}

ConfigReader::Section ConfigReader::ResolveSection(std::string_view line) {
  if (line == "[Discord]") {
    return Section::discord;
  }
  if (line == "[Audio]") {
    return Section::audio;
  }
  if (line == "[Net]") {
    return Section::net;
  }
  if (line == "[Video]") {
    return Section::video;
  }
  if (line == "[General]") {
    return Section::general;
  }
  if (line == "[Keyboard]") {
    return Section::keyboard;
  }
  if (line == "[Gamepad]") {
    return Section::gamepad;
  }

  return Section::none;
}

Gamepad ConfigReader::GetGamepadCode(int key) {
  return (Gamepad)key;
}

sf::Keyboard::Key ConfigReader::GetKeyCodeFromAscii(int ascii) {
  return (sf::Keyboard::Key)ascii;
}

// Parsing

bool ConfigReader::Parse(std::string_view view) {
  auto section = Section::none;
  auto isOk = true;

  while (true) {
    auto iter = std::find_first_of(view.begin(), view.end(), ::LINE_BREAKS.begin(), ::LINE_BREAKS.end());

    if (iter == view.end()) {
      break;
    }

    size_t endline = size_t(iter - view.begin());

    // get a view of the line
    std::string_view line = Trim(view.substr(0, endline));

    // push view to the next line
    view = view.substr(endline + 1);

    if (line.empty()) {
      // skip blank lines
      continue;
    }

    if (line[0] == '[') {
      section = ResolveSection(line);
      continue;
    }

    isOk = ParseProperty(section, line);
  }

  return isOk;
}

bool ConfigReader::ParseProperty(Section section, std::string_view line) {
  switch (section) {
  case Section::discord:
    return ParseDiscordProperty(line);
  case Section::audio:
    return ParseAudioProperty(line);
  case Section::net:
    return ParseNetProperty(line);
  case Section::video:
    return ParseVideoProperty(line);
  case Section::general:
    return ParseGeneralProperty(line);
  case Section::keyboard:
    return ParseKeyboardProperty(line);
  case Section::gamepad:
    return ParseGamepadProperty(line);
  }

  return true;
}

bool ConfigReader::ParseDiscordProperty(std::string_view line) {
  auto key = ResolveKey(line);
  auto value = ResolveValue(line);

  if (key == "User") {
    settings.discord.user = value;
    return true;
  }
  if (key == "Key") {
    settings.discord.key = value;
    return true;
  }

  auto keyCopy = std::string(key);
  Logger::Logf(LogLevel::warning, "Unknown config property in [Discord]: %s", keyCopy.c_str());

  return true;
}

bool ConfigReader::ParseAudioProperty(std::string_view line) {
  auto key = ResolveKey(line);
  auto value = std::string(ResolveValue(line));

  if (key == "Music") {
    settings.musicLevel = std::atoi(value.c_str());
    return true;
  }
  if (key == "SFX") {
    settings.sfxLevel = std::atoi(value.c_str());
    return true;
  }

  auto keyCopy = std::string(key);
  Logger::Logf(LogLevel::warning, "Unknown config property in [Audio]: %s", keyCopy.c_str());

  return true;
}

bool ConfigReader::ParseNetProperty(std::string_view line) {
  return true;
}

bool ConfigReader::ParseVideoProperty(std::string_view line) {
  auto key = ResolveKey(line);
  auto value = std::string(ResolveValue(line));

  if (key == "Fullscreen") {
    settings.fullscreen = value == "1" ? true : false;
    return true;
  }

  if (key == "Shader") {
    settings.shaderLevel = std::atoi(value.c_str());
    return true;
  }

  auto keyCopy = std::string(key);
  Logger::Logf(LogLevel::warning, "Unknown config property in [Video]: %s", keyCopy.c_str());

  return true;
}

bool ConfigReader::ParseGeneralProperty(std::string_view line) {
  auto key = ResolveKey(line);
  auto value = ResolveValue(line);

  if (key == "Invert Minimap") {
    auto valueCopy = std::string(value);
    settings.invertMinimap = std::atoi(valueCopy.c_str()) == 1;
    return true;
  }

  auto keyCopy = std::string(key);
  Logger::Logf(LogLevel::warning, "Unknown config property in [General]: %s", keyCopy.c_str());

  return true;
}

bool ConfigReader::ParseKeyboardProperty(std::string_view line) {
  auto key = ResolveKey(line);
  auto value = ResolveValue(line);

  for (auto eventName : InputEvents::KEYS) {
    if (key == eventName) {
      auto valueCopy = std::string(value);
      auto code = GetKeyCodeFromAscii(std::atoi(valueCopy.c_str()));

      settings.keyboard.insert(std::make_pair(code, key));
      return true;
    }
  }

  auto keyCopy = std::string(key);
  Logger::Logf(LogLevel::warning, "Unknown config property in [Keyboard]: %s", keyCopy.c_str());

  return true;
}

bool ConfigReader::ParseGamepadProperty(std::string_view line) {
  auto key = ResolveKey(line);
  auto value = ResolveValue(line);

  if (key == "Gamepad Index") {
    auto valueCopy = std::string(value);
    settings.gamepadIndex = std::atoi(valueCopy.c_str());
    return true;
  }

  if (key == "Invert Thumbstick") {
    auto valueCopy = std::string(value);
    settings.invertThumbstick = std::atoi(valueCopy.c_str()) == 1;
    return true;
  }

  for (auto eventName : InputEvents::KEYS) {
    if (key == eventName) {
      auto valueCopy = std::string(value);
      settings.gamepad.insert(std::make_pair(GetGamepadCode(std::atoi(valueCopy.c_str())), key));
      return true;
    }
  }

  auto keyCopy = std::string(key);
  Logger::Logf(LogLevel::warning, "Unknown config property in [Gamepad]: %s", keyCopy.c_str());

  return true;
}

bool ConfigReader::IsOK()
{
  return isOK;
}

ConfigReader::ConfigReader(const std::filesystem::path& filepath):
  filepath(filepath)
{
  settings.isOK = Parse(FileUtil::Read(filepath));

  // We need SOME values
  // Provide default layout
  if (!settings.isOK || !settings.TestKeyboard()) {
    settings.isOK = false; // This can be used as a flag that we're using default controls
  }
}

ConfigReader::~ConfigReader() { ; }

ConfigSettings ConfigReader::GetConfigSettings()
{
  return settings;
}

const std::filesystem::path ConfigReader::GetPath() const
{
  return filepath;
}
