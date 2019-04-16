#pragma once

#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/Music.hpp>
#include "bnAudioType.h"
#include <atomic>

// For more authentic retro experience, decrease available channels.
#define NUM_OF_CHANNELS 10

enum class AudioPriority : int {
  LOWEST,
  LOW,
  HIGH,
  HIGHEST
};

class AudioResourceManager {
public:
  static AudioResourceManager& GetInstance();

  void EnableAudio(bool status);
  void LoadAllSources(std::atomic<int> &status);
  void LoadSource(AudioType type, const std::string& path);
  int Play(AudioType type, AudioPriority priority = AudioPriority::LOW);
  int Stream(std::string path, bool loop = false);
  void StopStream();
  void SetStreamVolume(float volume);
  void SetChannelVolume(float volume);

  AudioResourceManager();
  ~AudioResourceManager();

private:
  struct Channel {
    sf::Sound buffer;
    AudioPriority priority;
  };

  Channel* channels;
  sf::SoundBuffer* sources;
  sf::Music stream;
  int channelVolume;
  int streamVolume;
  bool isEnabled;
};

#define AUDIO AudioResourceManager::GetInstance()
