#include "bnAudioResourceManager.h"
#include "bnLogger.h"

AudioResourceManager& AudioResourceManager::GetInstance() {
  static AudioResourceManager instance;
  return instance;
}

AudioResourceManager::AudioResourceManager() {
  isEnabled = true;

  channels = new AudioResourceManager::Channel[NUM_OF_CHANNELS];

  for (int i = 0; i < NUM_OF_CHANNELS; i++) {
    channels[i].buffer = sf::Sound();
  }

  sources = new sf::SoundBuffer[AudioType::AUDIO_TYPE_SIZE];

  for (int i = 0; i < AUDIO_TYPE_SIZE; i++) {
    sources[i] = sf::SoundBuffer();
  }

  channelVolume = streamVolume = 100; //SFML default
}


AudioResourceManager::~AudioResourceManager() {
  // Stop playing everything 
  stream.stop();

  for (int i = 0; i < NUM_OF_CHANNELS; i++) {
    channels[i].buffer.stop();
  }

  // Free memory
  delete[] channels;
  delete[] sources;
}

void AudioResourceManager::EnableAudio(bool status) {
  isEnabled = status;
}

void AudioResourceManager::LoadAllSources(std::atomic<int> &status) {
  LoadSource(AudioType::APPEAR, "resources/sfx/appear.ogg"); status++;
  LoadSource(AudioType::AREA_GRAB, "resources/sfx/area_grab.ogg"); status++;
  LoadSource(AudioType::AREA_GRAB_TOUCHDOWN, "resources/sfx/area_grab_touchdown.ogg"); status++;
  LoadSource(AudioType::BUSTER_PEA, "resources/sfx/pew.ogg"); status++;
  LoadSource(AudioType::BUSTER_CHARGED, "resources/sfx/buster_charged.ogg"); status++;
  LoadSource(AudioType::BUSTER_CHARGING, "resources/sfx/buster_charging.ogg"); status++;
  LoadSource(AudioType::GUARD_HIT, "resources/sfx/guard_hit.ogg"); status++;
  LoadSource(AudioType::CANNON, "resources/sfx/cannon.ogg"); status++;
  LoadSource(AudioType::COUNTER, "resources/sfx/counter.ogg"); status++;
  LoadSource(AudioType::CHIP_CANCEL, "resources/sfx/chip_cancel.ogg"); status++;
  LoadSource(AudioType::CHIP_CHOOSE, "resources/sfx/chip_choose.ogg"); status++;
  LoadSource(AudioType::CHIP_CONFIRM, "resources/sfx/chip_confirm.ogg"); status++;
  LoadSource(AudioType::CHIP_DESC, "resources/sfx/chip_desc.ogg"); status++;
  LoadSource(AudioType::CHIP_DESC_CLOSE, "resources/sfx/chip_desc_close.ogg"); status++;
  LoadSource(AudioType::CHIP_SELECT, "resources/sfx/chip_select.ogg"); status++;
  LoadSource(AudioType::CHIP_ERROR, "resources/sfx/chip_error.ogg"); status++;
  LoadSource(AudioType::CUSTOM_BAR_FULL, "resources/sfx/custom_bar_full.ogg"); status++;
  LoadSource(AudioType::CUSTOM_SCREEN_OPEN, "resources/sfx/chip_screen_open.ogg"); status++;
  LoadSource(AudioType::ITEM_GET, "resources/sfx/item_get.ogg"); status++;
  LoadSource(AudioType::DELETED, "resources/sfx/deleted.ogg"); status++;
  LoadSource(AudioType::EXPLODE, "resources/sfx/explode_once.ogg"); status++;
  LoadSource(AudioType::GUN, "resources/sfx/gun.ogg"); status++;
  LoadSource(AudioType::HURT, "resources/sfx/hurt.ogg"); status++;
  LoadSource(AudioType::PANEL_CRACK, "resources/sfx/panel_crack.ogg"); status++;
  LoadSource(AudioType::PANEL_RETURN, "resources/sfx/panel_return.ogg"); status++;
  LoadSource(AudioType::PAUSE, "resources/sfx/pause.ogg"); status++;
  LoadSource(AudioType::PRE_BATTLE, "resources/sfx/pre_battle.ogg"); status++;
  LoadSource(AudioType::RECOVER, "resources/sfx/recover.ogg"); status++;
  LoadSource(AudioType::SPREADER, "resources/sfx/spreader.ogg"); status++;
  LoadSource(AudioType::SWORD_SWING, "resources/sfx/sword_swing.ogg"); status++;
  LoadSource(AudioType::TOSS_ITEM, "resources/sfx/toss_item.ogg"); status++;
  LoadSource(AudioType::WAVE, "resources/sfx/wave.ogg"); status++;
  LoadSource(AudioType::THUNDER, "resources/sfx/thunder.ogg"); status++;
  LoadSource(AudioType::INVISIBLE, "resources/sfx/invisible.ogg"); status++;
  LoadSource(AudioType::PA_ADVANCE, "resources/sfx/pa_advance.ogg"); status++;
  LoadSource(AudioType::LOW_HP, "resources/sfx/low_hp.ogg"); status++;
  LoadSource(AudioType::POINT, "resources/sfx/point.ogg"); status++;
  LoadSource(AudioType::NEW_GAME, "resources/sfx/new_game.ogg"); status++;
  LoadSource(AudioType::TEXT, "resources/sfx/text.ogg"); status++;
}

void AudioResourceManager::LoadSource(AudioType type, const std::string& path) {
  if (!sources[type].loadFromFile(path)) {

    Logger::GetMutex()->lock();
    Logger::Logf("Failed loading audio: %s\n", path.c_str());
    Logger::GetMutex()->unlock();

  } else {

    Logger::GetMutex()->lock();
    Logger::Logf("Loaded audio: %s", path.c_str());
    Logger::GetMutex()->unlock();
  }
}

int AudioResourceManager::Play(AudioType type, AudioPriority priority) {
  if (!isEnabled) { return -1; }

  if (type < AudioType(0) || type >= AudioType::AUDIO_TYPE_SIZE) {
    return -1;
  }


  // Priorities are LOWEST  (one at a time, if channel available),
  //                LOW     (any free channels),
  //                HIGH    (force a channel to play sound, but one at a time, and don't interrupt other high priorities),
  //                HIGHEST (force a channel to play sound always)


  // Highest priority plays over anything that isn't like it
  if (priority == AudioPriority::HIGHEST) {
    for (int i = 0; i < NUM_OF_CHANNELS; i++) {
      if (channels[i].buffer.getStatus() != sf::SoundSource::Status::Playing || (sf::SoundBuffer*)channels[i].buffer.getBuffer() != &sources[type]) {
        channels[i].buffer.stop();
        channels[i].buffer.setBuffer(sources[type]);
        channels[i].buffer.play();
        channels[i].priority = priority;
        return 0;
      }
    }
  }

  // For lowest priority or high priority sounds, scan and see if this sound is already playing...
  // This step is also used as a pre-check for low priority sounds
  if (priority == AudioPriority::LOWEST || priority == AudioPriority::HIGH) {
    for (int i = 0; i < NUM_OF_CHANNELS; i++) {
      if (channels[i].buffer.getStatus() == sf::SoundSource::Status::Playing) {
        if ((sf::SoundBuffer*)channels[i].buffer.getBuffer() == &sources[type]) {
          // Lowest priority or high priority sounds only play once 
          return -1;
        }
      }
    }
  }

  // Play sound based on priortiy rules
  for (int i = 0; i < NUM_OF_CHANNELS; i++) {
    if (priority != AudioPriority::HIGH) {
      if (channels[i].buffer.getStatus() != sf::SoundSource::Status::Playing) {
        channels[i].buffer.setBuffer(sources[type]);
        channels[i].buffer.play();
        channels[i].priority = priority;
        return 0;
      }
    }
    else { // HIGH PRIORITY
      if (channels[i].priority < AudioPriority::HIGH || channels[i].buffer.getStatus() != sf::SoundSource::Status::Playing) {
        channels[i].buffer.stop();
        channels[i].buffer.setBuffer(sources[type]);
        channels[i].buffer.play();
        channels[i].priority = priority;
        return 0;
      }
    }
  }

  // No free channel? Skip playing this sound.
  return -1;
}

int AudioResourceManager::Stream(std::string path, bool loop) {
  if (!isEnabled) { return -1; }

  // stop previous stream if any 
  stream.stop();

  if (!stream.openFromFile(path))
    return -1; // error

  stream.play();
  stream.setLoop(loop);

  return 0;
}

void AudioResourceManager::StopStream() {
  stream.stop();
}

void AudioResourceManager::SetStreamVolume(float volume) {
  stream.setVolume(volume);
}

void AudioResourceManager::SetChannelVolume(float volume) {
  for (int i = 0; i < NUM_OF_CHANNELS; i++) {
    channels[i].buffer.setVolume(volume);
  }
}
