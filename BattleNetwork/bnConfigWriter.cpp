#include "bnConfigWriter.h"
#include "bnFileUtil.h"
#include "bnInputManager.h"
ConfigWriter::ConfigWriter(ConfigSettings & settings) : settings(settings)
{
}

ConfigWriter::~ConfigWriter()
{
}

void ConfigWriter::Write(std::string path)
{
  FileUtil::WriteStream w(path);
  w << "[Discord]" << w.endl();
  w << "User=" << "\"" << settings.GetDiscordInfo().user << "\"" << w.endl();
  w << "Key=" << "\"" << settings.GetDiscordInfo().key << "\"" << w.endl();
  w << "[Audio]" << w.endl();
  w << "Music=" << "\"" << std::to_string(settings.GetMusicLevel()) <<  "\""<< w.endl();
  w << "SFX=" << "\"" << std::to_string(settings.GetSFXLevel()) <<  "\"" << w.endl();
  w << "[Net]" << w.endl();
  w << "uPNP=" << "\"0\"" << w.endl();
  w << "[Video]" << w.endl();
  w << "Fullscreen=" << "\"0\"" << w.endl();
  w << "[Keyboard]" << w.endl();

  for (auto a : EventTypes::KEYS) {
    w << a << "=" << "\"" << std::to_string(GetAsciiFromKeyboard(settings.GetPairedInput(a))) << "\"" << w.endl();
  }

  w << "[Gamepad]" << w.endl();

  for (auto a : EventTypes::KEYS) {
    w << a << "=" << "\"" << std::to_string(GetAsciiFromGamepad(settings.GetPairedGamepadButton(a))) << "\"" << w.endl();
  }
}

int ConfigWriter::GetAsciiFromGamepad(Gamepad code)
{
  return (int)code;
}

int ConfigWriter::GetAsciiFromKeyboard(sf::Keyboard::Key key)
{
  return (int)key;
}
