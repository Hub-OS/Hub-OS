#pragma once
class Character;
class CharacterDeletePublisher;

/**
 * @class CharacterDeleteListener
 * @author mav
 * @date 12/28/19
 * @brief Respond to a character delete event
 *
 * Some attacks, enemies, or scene logic respond to deletions from Characters
 */
class CharacterDeleteListener {
public:
  CharacterDeleteListener() = default;
  ~CharacterDeleteListener() = default;

  CharacterDeleteListener(const CharacterDeleteListener& rhs) = delete;
  CharacterDeleteListener(CharacterDeleteListener&& rhs) = delete;

  /**
   * @brief Describe what happens when recieving delete events
   * @param pending Character who will be deleted after this function finishes
   */
  virtual void OnDeleteEvent(Character& pending) = 0;

  /**
   * @brief Subscribe to a potential publisher
   * @param publisher source that a counter event can emit from
   */
  void Subscribe(CharacterDeletePublisher& publisher);
}; 