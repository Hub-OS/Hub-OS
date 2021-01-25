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
  w << "User=" << quote(settings.GetDiscordInfo().user) << w.endl();
  w << "Key=" << quote(settings.GetDiscordInfo().key) << w.endl();
  w << "[Audio()]" << w.endl();
  w << "Music=" << quote(std::to_string(settings.GetMusicLevel())) << w.endl();
  w << "SFX=" << quote(std::to_string(settings.GetSFXLevel())) << w.endl();
  w << "[Net]" << w.endl();
  w << "WebServer=" << quote(settings.GetWebServerInfo().URL) << w.endl();
  w << "Version=" << quote(settings.GetWebServerInfo().version) << w.endl();
  w << "Port=" << quote(std::to_string(settings.GetWebServerInfo().port)) << w.endl();
  w << "Username=" << quote(settings.GetWebServerInfo().user) << w.endl();
  w << "Password=" << quote(settings.GetWebServerInfo().password) << w.endl();
  w << "[Video]" << w.endl();
  w << "Fullscreen=" << quote("0") << w.endl();
  w << "[Keyboard]" << w.endl();

  for (auto&& a : InputEvents::KEYS) {
    w << a << "=" << quote(std::to_string(GetAsciiFromKeyboard(settings.GetPairedInput(a)))) << w.endl();
  }

  w << "[Gamepad]" << w.endl();

  for (auto&& a : InputEvents::KEYS) {
    w << a << "=" << quote(std::to_string(GetAsciiFromGamepad(settings.GetPairedGamepadButton(a)))) << w.endl();
  }
}

const std::string ConfigWriter::quote(const std::string & str)
{
    return std::string() + "\"" + str + "\"";
}

int ConfigWriter::GetAsciiFromGamepad(Gamepad code)
{
  return (int)code;
}

int ConfigWriter::GetAsciiFromKeyboard(sf::Keyboard::Key key)
{
  return (int)key;
}
