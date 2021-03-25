
/*! \brief A bright pink shot effect that shoots across the map dealing damage */

#pragma once
#include "bnSpell.h"
#include "bnAnimation.h"

class RowHit : public Spell {
protected:
  Animation animation; /*!< the animation of the shot */
  int damage; /*!< How much damage to deal */
public:
  /**
   * @brief Sets the animation with 2 callbacks: spawn another RowHit and delete on animation end
   */
  RowHit(Team _team,int damage);
  
  /**
   * @brief Deconstructor
   */
  ~RowHit();

  /**
   * @brief Attack the tile and animate
   * @param _elapsed
   */
  void OnUpdate(double _elapsed) override;
  
  /**
   * @brief Deals damage with default hit props
   * @param _entity
   */
  void Attack(Character* _entity) override;

  /** 
  * @brief Does nothing
  */
  void OnDelete() override;
};
