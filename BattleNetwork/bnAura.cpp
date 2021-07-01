#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnSpell.h"
#include "bnAura.h"
#include "bnLayered.h"
#include "bnDefenseAura.h"
#include "battlescene/bnBattleSceneBase.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/spells/auras.animation"

void Aura::OnHitCallback(Spell& in, Character& owner, bool windRemove) {
  auto hitbox = in.GetHitboxProperties();
  if (windRemove) {
    if (fx && fx->flyStartTile == nullptr) {
      // Attach visual fx node to the tile and make it fly up and away based on direction
      auto direction = in.GetDirection();
      if (direction == Direction::none) {
        direction = owner.GetDirection();
      }

      if (direction == Direction::left) {
        this->fx->flyAccel = { -5.f, -12.0f };
      }
      else {
        this->fx->flyAccel = { 5.f, -12.0f };
      }

      owner.RemoveNode(this->fx);

      this->fx->flyStartTile = owner.GetTile();
      this->fx->flyStartTile->AddNode(this->fx);
      this->fx->SetLayer(-1); // stay in front of the tile

      // this will trigger the flicker-out animation as it flies away
      // and at the end of the flicker-out animation, it will cleanup memory
      this->health = this->fx->currHP = 0;
      this->fx = nullptr;
    }
    else {
      // We can no longer protect this user..
      owner.Hit(hitbox);
    }
  }
  else {
    TakeDamage(owner, in.GetHitboxProperties());
  }
};


Aura::Aura(Aura::Type type, Character* owner) :
  type(type),
  Component(owner, Component::lifetimes::battlestep),
  DefenseAura([this](Spell& s, Character& c, bool b) { OnHitCallback(s, c, b); })
{
  timer = 50; // seconds

  persist = false;
  isOver = false;

  switch (type) {
  case Aura::Type::AURA_100:
    health = 100;
    break;
  case Aura::Type::AURA_200:
    health = 200;
    break;
  case Aura::Type::AURA_1000:
    health = 1000;;
    break;
  case Aura::Type::BARRIER_10:
    health = 100;
    persist = true;
    break;
  case Aura::Type::BARRIER_200:
    health = 200;
    persist = true;
    break;
  case Aura::Type::BARRIER_500:
    health = 500;
    persist = true;
    break;
  }

  currHP = health;

  owner->AddDefenseRule(this);
  fx = owner->CreateComponent<Aura::VisualFX>(owner, type);
  fx->currHP = health;
  fx->timer = timer;
  owner->AddNode(fx);

  fx->ShowHP(owner->GetTeam() == Team::red); // red team sees their aura hp at all times
}

void Aura::OnUpdate(double _elapsed) {
  if (Injected() == false || IsReplaced()) return;

  if (fx && GetOwner()->GetTeam() == Team::blue) {
    if (Input().Has(InputEvents::held_option)) {
      fx->ShowHP(true);
    }
    else {
      fx->ShowHP(false);
    }
  }

  currHP = health;
  
  // The timer will be used for non-persistent aura types
  // And again when the aura is due for deletion (flicker time)
  // Though it should only lose time during battle steps
  if(!persist || isOver) {
    timer -= _elapsed;
  }

  if (!isOver) {
    if (health == 0 || timer <= 0.0) {
      isOver = true;
      timer = 2;
    }
  }
  else if (timer <= 0.0) {
    if (auto character = GetOwnerAs<Character>()) {
      character->RemoveDefenseRule(this);
    }

    Eject();
    return;
  }

 if (GetOwner()->GetTile() == nullptr) {
   fx? fx->Hide() : (void)0;
   return;
 }

 if (GetOwner()->WillRemoveLater()) {
   timer = 0;
   Eject();
 }

 if (fx) {
   // Update the visual fx
   fx->currHP = currHP;
 }
}

void Aura::Inject(BattleSceneBase& bs)
{
  bs.Inject(this);
}

void Aura::OnReplace()
{
  if (fx) {
    // Stop drawing the old aura
    GetOwner()->RemoveNode(fx);
  }

  Eject();
}

const Aura::Type Aura::GetAuraType()
{
  return type;
}
 
void Aura::Persist(bool enable) {
  persist = enable;
}

const bool Aura::IsPersistent() const {
   return persist;
 }
  
const int Aura::GetHealth() const {
  return health;
}

void Aura::TakeDamage(Character& owner, const Hit::Properties& props)
{
  if (health > 0) {
    if (type >= Aura::Type::BARRIER_10) {
      health = health - props.damage;
    }
    else if (health <= props.damage) {
      health = 0;
    }

    health = std::max(0, health);
  }
  else {
    // barrier cannot protect player, propagate hit to player
    owner.Hit(props);
  }
}

Aura::~Aura()
{
  if(fx) {
    GetOwner()->RemoveNode(fx);
    fx->Eject();
  }
}

//////////////////////////////////////////////////////
//                Visual FX object                  //
//////////////////////////////////////////////////////
Aura::VisualFX::VisualFX(Entity* owner, Aura::Type type) : 
  type(type),
  UIComponent(owner) {
    
  ResourceHandle handle;
  auraSprite.setTexture(*handle.Textures().GetTexture(TextureType::SPELL_AURA));
  aura = new SpriteProxyNode(auraSprite);
  SetLayer(1); // behind player

  fontTextureRef = LOAD_TEXTURE(AURA_NUMSET);
  font.setTexture(*fontTextureRef);

  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  switch (type) {
  case Aura::Type::AURA_100:
    animation.SetAnimation("AURA_100");
    break;
  case Aura::Type::AURA_200:
    animation.SetAnimation("AURA_200");
    break;
  case Aura::Type::AURA_1000:
    animation.SetAnimation("AURA_1000");
    break;
  case Aura::Type::BARRIER_10:
    animation.SetAnimation("BARRIER_10");
    break;
  case Aura::Type::BARRIER_200:
    animation.SetAnimation("BARRIER_200");
    break;
  case Aura::Type::BARRIER_500:
    animation.SetAnimation("BARRIER_500");
    break;
  }

  animation << Animator::Mode::Loop;
  animation.Update(0, aura->getSprite());

  AddNode(aura);
  this->setPosition(0, -owner->GetHeight() / 2.f / 2.f); // descale from 2.0x upscale and then get the half of the height

  // we want to draw relative to the component's owner
  SetDrawOnUIPass(false);
}

Aura::VisualFX::~VisualFX() {
  if (flyStartTile) {
    flyStartTile->RemoveNode(this);
  }

  delete aura;
}

void Aura::VisualFX::draw(sf::RenderTarget& target, sf::RenderStates states) const {
  auto this_states = states;
  this_states.transform *= getTransform();

  const float* t = this_states.transform.getMatrix();

  sf::Transform aura_transform = sf::Transform(2.f, t[4], t[12], t[1], 2.f, t[13], t[3], t[7], t[15]);
  this_states.transform = aura_transform;

  UIComponent::draw(target, this_states);

  // Only draw HP font for Barriers. Auras are hidden.
  if (type <= Type::AURA_1000 || !showHP) return;

  // 0 - 5 are on first row
  // Glyphs are 8x15 
  // 2nd row starts +1 pixel down

  if (currHP > 0) {
    int size = (int)(std::to_string(currHP).size());
    int hp = currHP;
    float offsetx = -size * 2.0f;
    float flip = 1.f;
    int index = 0;
    font.setScale(1.f, 1.f);

    while (index < size) {
      const char c = std::to_string(currHP)[index];
      int number = std::atoi(&c);
      int rowstart = 0;

      if (number > 4) {
        rowstart = 16;
      }

      int col = 8 * (number % 5);

      font.setTextureRect(sf::IntRect(col, rowstart, 8, 15));
      font.setPosition(sf::Vector2f(offsetx, 7.0f));

      auto font_states = this_states;
      font_states.shader = nullptr; // don't allow shader passes to effect this font...
      font_states.transform *= font.getTransform();

      target.draw(font, font_states);

      offsetx += 4.0f;

      index++;
    }
  }
}

void Aura::VisualFX::Inject(BattleSceneBase& bs) {
  bs.Inject((Component*)this);
  bs.Audio().Play(AudioType::APPEAR);
}

void Aura::VisualFX::OnUpdate(double _elapsed)
{
  timer += _elapsed;

  // accumulate acceleration if nonzero
  flySpeed.x += flyAccel.x * float(_elapsed);
  flySpeed.y += flyAccel.y * float(_elapsed);

  sf::Vector2f newPos = getPosition();
  newPos.x += flySpeed.x;
  newPos.y += flySpeed.y;

  setPosition(newPos);

  // Barriers flicker every 3rd frame
  if (type > Type::AURA_1000) {
    if (from_seconds(timer).count() % 4 == 0) {
      Hide();
    }
    else {
      Reveal();
    }
  }

  if (currHP == 0) {
    if (!flyStartTile) {
      auraSprite.setColor(sf::Color(255, 255, 255, 50));
    }
    else if (currHP == 0 && (flyStartTile->getPosition().y + getPosition().y) < 0) {
      // we have flown up and out of the screen
      Eject();
    }
  }

  animation.Update(_elapsed, aura->getSprite());
}

void Aura::VisualFX::ShowHP(bool visible)
{
  showHP = visible;
}
