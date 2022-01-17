#include "bnConfigWriter.h"
#include "bnFileUtil.h"
#include "bnInputManager.h"
ConfigWriter::ConfigWriter(ConfigSettings& settings) : settings(settings)
{
}

ConfigWriter::~ConfigWriter()
{
}

void ConfigWriter::Write(const std::filesystem::path& path)
{
  FileUtil::WriteStream w(path);
  w << "[Discord]" << w.endl();
  w << "User=" << quote(settings.GetDiscordInfo().user) << w.endl();
  w << "Key=" << quote(settings.GetDiscordInfo().key) << w.endl();
  w << "[Audio]" << w.endl();
  w << "Music=" << quote(std::to_string(settings.GetMusicLevel())) << w.endl();
  w << "SFX=" << quote(std::to_string(settings.GetSFXLevel())) << w.endl();
  w << "[Net]" << w.endl();
  w << "[Video]" << w.endl();
  w << "Fullscreen=" << quote("0") << w.endl();
  w << "Shader=" << quote(std::to_string(settings.GetShaderLevel())) << w.endl();
  w << "[General]" << w.endl();
  w << "Invert Minimap=" << quote(std::to_string(settings.GetInvertMinimap())) << w.endl();
  w << "[Keyboard]" << w.endl();

  for (auto&& a : InputEvents::KEYS) {
    w << a << "=" << quote(std::to_string(GetAsciiFromKeyboard(settings.GetPairedInput(a)))) << w.endl();
  }

  w << "[Gamepad]" << w.endl();
  w << "Gamepad Index=" << quote(std::to_string(settings.GetGamepadIndex())) << w.endl();
  w << "Invert Thumbstick=" << quote(std::to_string(settings.GetInvertThumbstick())) << w.endl();

  for (auto&& a : InputEvents::KEYS) {
    w << a << "=" << quote(std::to_string(GetAsciiFromGamepad(settings.GetPairedGamepadButton(a)))) << w.endl();
  }
}

const std::string ConfigWriter::quote(const std::string& str)
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
