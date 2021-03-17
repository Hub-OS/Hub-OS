#pragma once
#include "bnSpell.h"
#include "bnAnimation.h"

class FireBurn : public Spell {
public:
  enum class Type : int {
    _1 = 1,
    _2 = 2,
    _3 = 3
  };

protected:
  Animation animation; /*!< the animation of the shot */
  int damage; /*!< How much damage to deal */
  bool crackTiles{ true };
public:
  FireBurn(Team _team,Type type, int damage);

  /**
   * @brief Deconstructor
   */
  ~FireBurn();

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
  * @brief does nothing
  */
  void OnDelete() override;

  void CrackTiles(bool state);
};
