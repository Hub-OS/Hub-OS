#pragma once
#include "bnMegalianIdleState.h"
#include "bnAI.h"
#include "bnCharacter.h"
#include "bnObstacle.h"
#include "bnTextureType.h"
#include "bnMobHealthUI.h"
#include "bnAnimationComponent.h"
#include "bnAudioResourceManager.h"
#include "bnTextureResourceManager.h"
#include <Swoosh\Ease.h>

/*! \brief Megalian enemy is composed of two characters: one deals damage and propogates all damage, and the other controls the whole */
class Megalian : public Character, public AI<Megalian> {
  friend class MegalianIdleState;

private:
  class Head : public Obstacle {
  public:
    Head(Megalian* belongsTo) : base(belongsTo), Obstacle(belongsTo->GetField(), belongsTo->GetTeam()) {
      this->SetFloatShoe(true);
      this->SetTeam(base->GetTeam());

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
      this->SetDirection(Direction::LEFT);
      this->SetSlideTime(sf::milliseconds(250));
      timer = 0;

      this->ShareTileSpace(true);

      auto props = this->GetHitboxProperties();
      props.damage = 30;
      props.flags = Hit::impact | Hit::breaking;
      props.aggressor = base;
      this->SetHitboxProperties(props);
    }

    virtual ~Head() { }

    virtual void OnUpdate(float _elapsed) { 
      if (!base || base->IsDeleted()) {
        this->Delete();
        return;
      }
      auto baseOffset = base->GetFirstComponent<AnimationComponent>()->GetPoint("head");
      auto origin = base->operator sf::Sprite &().getOrigin();

      // transform from sprite space to world space -- scale by 2
      baseOffset = baseOffset - origin;
      baseOffset = sf::Vector2f(2.0f*baseOffset.x, 2.0f*baseOffset.y);

      setPosition(tile->getPosition().x, tile->getPosition().y);
      setPosition(getPosition() + baseOffset + tileOffset);

      this->tile->AffectEntities(this);

      if (!this->IsSliding()) {
        timer += _elapsed;

        if (timer >= 5) {
          static bool playOnce = true;
          //auto adjusted = -swoosh::ease::wideParabola(timer - 5.0, 250.0, 1.0);
          //this->SetSlideTime(sf::milliseconds(adjusted));

          if(playOnce) {
            AUDIO.Play(AudioType::TOSS_ITEM_LITE);
            playOnce = false;
          }

          if (this->GetTile() == base->GetTile() && this->GetDirection() == Direction::RIGHT) {
            this->SetDirection(Direction::LEFT);
            timer = 0;
            animation->Reload();
            animation->SetAnimation("Head1");
            animation->SetPlaybackSpeed(0);
            playOnce = true;
          }
          else {

            this->SlideToTile(true);
            this->Move(this->GetDirection());
            
            if(this->GetDirection() == Direction::NONE) {
              this->SetDirection(Direction::RIGHT);
            }
          }
        }

      }
    }

    virtual const bool OnHit(const Hit::Properties props) {
      if (GetTile() == base->GetTile()) return false;

      auto newProps = props;
      newProps.flags |= Hit::pierce;

      base->SetHealth(base->GetHealth() - props.damage);
      
      return true;
    }
    virtual void OnDelete() { }
    virtual const float GetHitHeight() const { return 15; }
    virtual bool CanMoveTo(Battle::Tile * next) { return true;  }

    virtual void Attack(Character* e) {
      e->Hit(this->GetHitboxProperties());
    }

  protected:
    AnimationComponent* animation;
    Megalian* base;
    double timer;
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