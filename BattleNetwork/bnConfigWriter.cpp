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
  /*
  [Discord]
  User="JohnDoe"
  Key="ABCDE"
  [Audio]
  Play="1"
  [Net]
  uPNP="0"
  [Video]
  Fullscreen="0"
  [Keyboard]
*/

  FileUtil::WriteStream w(path);
  w << "[Discord]" << w.endl();
  w << "User=" << "\"" << settings.GetDiscordInfo().user << "\"" << w.endl();
  w << "Key=" << "\"" << settings.GetDiscordInfo().key << "\"" << w.endl();
  w << "[Audio]" << w.endl();
  w << "Play=" << (settings.IsAudioEnabled() ? "\"1\"" : "\"0\"") << w.endl();
  w << "[Net]" << w.endl();
  w << "uPNP=" << "\"0\"" << w.endl();
  w << "[Video]" << w.endl();
  w << "Fullscreen=" << "\"0\"" << w.endl();
  w << "[Keyboard]" << w.endl();

  for (auto a : EventTypes::KEYS) {
    w << a << "=" << "\"" << std::to_string(GetAsciiFromKeyboard(settings.GetPairedInput(a))) << "\"" << w.endl();
  }
}

int ConfigWriter::GetAsciiFromGamepad(ConfigSettings::Gamepad code)
{
  return (int)code;
}

int ConfigWriter::GetAsciiFromKeyboard(sf::Keyboard::Key key)
{
  return (int)key;
}
