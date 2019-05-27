#pragma once

#include "bnEntity.h"
<<<<<<< HEAD

class BattleScene;
=======
>>>>>>> b486e21e11627262088deae73097eaa7af56791c

class BattleScene;

/**
 * @class Component
 * @author mav
 * @date 05/05/19
 * @brief Components need to be genric enough to be used by all in-battle entities
 *
 * Components can be attached to (and owned by) any Entity in the game
 * This allows for custom behavior on pre-existing effects, characters, and attacks
 */
class Component {
private: 
<<<<<<< HEAD
  Entity* owner;
  static long numOfComponents;
  long ID;

public:
  Component() = delete;
=======
  Entity* owner; /*!< Who the component is attached to */
  static long numOfComponents; /*!< Resource counter to generate new IDs */
  long ID; /*!< ID for quick lookups, resource management, and scripting */

public:
  Component() = delete;
  
  /**
   * @brief Sets an owner and ID. Increments numOfComponents beforehand.
   * @param owner the entity to attach to
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  Component(Entity* owner) { this->owner = owner; ID = ++numOfComponents;  };
  virtual ~Component() { ; }

  Component(Component&& rhs) = delete;
  Component(const Component& rhs) = delete;

<<<<<<< HEAD
  Entity* GetOwner() { return owner;  }

  template<typename T>
  T* GetOwnerAs() { return dynamic_cast<T*>(owner); }

  void FreeOwner() { owner = nullptr; }

  const long GetID() const { return ID; }

  virtual void Update(float _elapsed) = 0;
=======
  /**
   * @brief Get the owner as an Entity
   * @return Entity*
   */
  Entity* GetOwner() { return owner;  }

  /**
   * @brief Get the owner as a special type
   * @return T* if dynamic_cast is successful, otherwise null if type T is incompatible
   */
  template<typename T>
  T* GetOwnerAs() { return dynamic_cast<T*>(owner); }

  /**
   * @brief Releases the pointer and sets it to null
   * 
   * Useful to identify if an entity has been removed from game but a component
   * needs to act accordingly to this information
   */
  void FreeOwner() { owner = nullptr; }

  /**
   * @brief Get the Id of the component
   * @return ID
   */
  const long GetID() const { return ID; }

  /**
   * @brief Update must be implemented by child class
   * @param _elapsed in seconds
   */
  virtual void Update(float _elapsed) = 0;
  
  /**
   * @brief Some components can be injected into the battle routine for custom behavior
   * 
   * Some components modify the battle routine and the battle scene needs to know about their
   * existence.
   * 
   * Some components need to exist in the global update loop outside of the Entity's update loop,
   * and injcting is a great way to ensure the component is updated with the scene.
   * 
   * @warning Components injected into the battle scene are updated and deleted. Free the owner if injecting.
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  virtual void Inject(BattleScene&) = 0;
};
