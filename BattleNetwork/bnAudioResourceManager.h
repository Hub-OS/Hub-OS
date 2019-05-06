#pragma once

#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/Music.hpp>
#include "bnAudioType.h"
#include <atomic>

// For more retro experience, decrease available channels.
#define NUM_OF_CHANNELS 10

/**
  * @class AudioPriority
  * @brief Each priority describes how or if a playing sample should be interrupted
  * 
  * Priorities are LOWEST  (one at a time, if channel available),
  *                LOW     (any free channels),
  *                HIGH    (force a channel to play sound, but one at a time, and don't interrupt other high priorities),
  *                HIGHEST (force a channel to play sound always)
  */
enum class AudioPriority : int {
  LOWEST,
  LOW,
  HIGH,
  HIGHEST
};

/**
 * @class AudioResourceManager
 * @author mav
 * @date 06/05/19
 * @file bnAudioResourceManager.h
 * @brief Singleton loads audio samples
 */
class AudioResourceManager {
public:
  /**
   * @brief If first call, initializes audio resource instance and returns
   * @return AudioResourceManager&
   */
  static AudioResourceManager& GetInstance();

  /**
   * @brief If true, plays audio. If false, mute all audio
   * @param status
   */
  void EnableAudio(bool status);
  
  /**
   * @brief Loads all queued resources. Increases status value.
   * @param status thread-safe counter will reach total count of all samples to load when finished.
   */
  void LoadAllSources(std::atomic<int> &status);
  
  /**
   * @brief Loads an audio source at path and map it to enum type
   * @param type audio enum to map to
   * @param path path to audio sample
   */
  void LoadSource(AudioType type, const std::string& path);
  
  /**
   * @brief Play a sound with an audio priority
   * @param type audio to play
   * @param priority describes if and how to interrupt other playing samples
   * @return -1 if could not play, otherwise 0
   */
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
