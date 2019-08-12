#pragma once
#include "bnMegalianIdleState.h"
#include "bnAI.h"
#include "bnCharacter.h"
#include "bnObstacle.h"
#include "bnTextureType.h"
#include "bnMobHealthUI.h"
#include "bnAnimationComponent.h"
#include "bnTextureResourceManager.h"

/*! \brief Megalian enemy is composed of two characters: one deals damage and propogates all damage, and the other controls the whole */
class Megalian : public Character, public AI<Megalian> {
  friend class MegalianIdleState;

private:
  class Head : public Obstacle {
  public:
    Head(Megalian* belongsTo) : base(belongsTo), Obstacle(belongsTo->GetField(), belongsTo->GetTeam()) {
      animation = new AnimationComponent(this);
      auto baseAnimation = belongsTo->GetFirstComponent<AnimationComponent>();
      auto str = baseAnimation->GetFilePath();
      animation->Setup(str);
      animation->Load();
      animation->SetAnimation("Head1");
      animation->SetPlaybackSpeed(0); 
      setScale(2.f, 2.f);
      setTexture(*TEXTURES.GetTexture(TextureType::MOB_MEGALIAN_ATLAS));
      animation->OnUpdate(0);
      this->SetLayer(-1); // on top of base
      this->SetHealth(1);
      this->RegisterComponent(animation);

    }

    virtual ~Head() { }

    virtual void OnUpdate(float _elapsed) { 
      tileOffset = base->GetFirstComponent<AnimationComponent>()->GetPoint("head");
      auto origin = base->operator sf::Sprite &().getOrigin();

      // transform from sprite space to world space -- scale by 2
      tileOffset = tileOffset - origin;
      tileOffset = sf::Vector2f(2.0f*tileOffset.x, 2.0f*tileOffset.y);

      setPosition(base->getPosition().x, base->getPosition().y);
      setPosition(getPosition() + tileOffset);
    }

    virtual const bool OnHit(const Hit::Properties props) {
      if (GetTile() == base->GetTile()) return false;

      base->Hit(props);

      return false;
    }
    virtual void OnDelete() { }
    virtual const float GetHitHeight() const { return 15; }
    virtual bool CanMoveTo(Battle::Tile * next) { return true;  }

    virtual void Attack(Character* e) {

    }

  protected:
    AnimationComponent* animation;
    Megalian* base;
  };

public:
  using DefaultState = MegalianIdleState;

  /**
 * @brief Loads animations and gives itself a turn ID
 */
  Megalian(Rank _rank = Rank::_1);

  /**
   * @brief Removes its turn ID from the list of active mettaurs
   */
  virtual ~Megalian();

  /**
   * @brief Uses AI state to move around. Deletes when health is below zero.
   * @param _elapsed in seconds
   */
  virtual void OnUpdate(float _elapsed);

  /**
   * @brief Takes damage and flashes white
   * @param props
   * @return true if hit, false if missed
   */
  virtual const bool OnHit(const Hit::Properties props);

  virtual void OnDelete();

  /**
   * @brief Get the hit height of this entity
   * @return const float
   */
  virtual const float GetHitHeight() const;

private:

  AnimationComponent* animationComponent;
  float hitHeight; /*!< hit height */
  TextureType textureType;
  Head* head;
};