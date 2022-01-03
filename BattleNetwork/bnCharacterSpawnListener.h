#pragma once
#include <memory>
#include "bnCharacter.h"
class CharacterSpawnPublisher;

/**
 * @class CharacterSpawnListener
 * @author mav
 * @date 12/28/19
 * @brief Respond to a character spawn event
 *
 * Some events and the battle's mob list respond to spawn events
 */
class CharacterSpawnListener {
public:
  CharacterSpawnListener() = default;
  ~CharacterSpawnListener() = default;

  CharacterSpawnListener(const CharacterSpawnListener& rhs) = delete;
  CharacterSpawnListener(CharacterSpawnListener&& rhs) = delete;

  /**
   * @brief Describe what happens when recieving spawn events
   * @param spawned Character who has just spawned (before this function is reached)
   */
  virtual void OnSpawnEvent(std::shared_ptr<Character>& spawned) = 0;

  /**
   * @brief Subscribe to a potential publisher
   * @param publisher source that the event can emit from
   */
  void Subscribe(CharacterSpawnPublisher& publisher);
}; 