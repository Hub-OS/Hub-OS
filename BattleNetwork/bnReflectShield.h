
/*! \brief Component that protects an entity from all contact damage
 * 
 * The shield is just an animation but it adds a defense rule 
 * to the attached entity.
 * 
 * Defense rules check against all attacks before resolving damage
 * If the rule passes, the attack hits
 * In this case, Reflect Shield protects all tangible damage
 * and will return true, firing a callback.
 * 
 * This callback will spawn a RowHit spell object to deal damage
 */

#pragma once
#include "bnSpriteProxyNode.h"
#include "bnArtifact.h"
#include "bnField.h"

class ReflectShield : public Artifact
{
private:
  DefenseRule* guard; /*!< Adds defense rule to attached entity */
  Animation animation; /*!< Shield animation */
  bool activated; /*!< Flag if effect is active */
  int damage; /*!< Damage the reflect trail deals */
  Character* owner{ nullptr };
public:
  /**
   * @brief Adds a guard rule to the attached entity for a short time *
   * 
   * A guard rule object is constructed. When it fails it runs DoReflect()
   * which spawns a RowHit spell
   * 
   * At the end of the shield animation the rule is dropped and this 
   * component is removed from the owner and then deleted.
   */
  ReflectShield(Character* owner, int damage);
  
  /**
   * @brief Delete guard pointer
   */
  ~ReflectShield();
  
  /**
   * @brief Updates animation
   * @param _elapsed
   */
  void OnUpdate(float _elapsed) override;
  
  /**
   * @brief Removes reflect shield from play
   */
  void OnDelete() override;

  /**
   * @brief This artifact does not move across the grid
   * @param _direction ignored
   * @return false
   */
  bool Move(Direction _direction) override;

  /**
   * @brief If the first time reflecting, spawn a RowHit spell
   * @param in the attack we are reflecting
   * @param owner the owner of the attack
   */
  void DoReflect(Spell& in, Character& owner);
};
