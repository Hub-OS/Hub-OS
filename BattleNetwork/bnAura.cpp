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

Aura::Aura(Aura::Type type, Character* owner) : type(type), Component(owner, Component::lifetimes::battlestep)
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

  DefenseAura::Callback onHit = [this](Spell& in, Character& owner) {
    TakeDamage(in.GetHitboxProperties().damage);
  };
  
  defense = new DefenseAura(onHit);
  
  owner->AddDefenseRule(defense);
  fx = owner->CreateComponent<Aura::VisualFX>(owner, type);
  fx->currHP = health;
  fx->timer = timer;
  owner->AddNode(fx);
}

void Aura::OnUpdate(float _elapsed) {
  if (Injected() == false) return;

  // If the aura has been replaced by another defense rule, remove all
  // associated components 
  if (defense->IsReplaced()) {
    fx->Eject();
    GetOwner()->RemoveNode(fx);
    Eject();
    return;
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
    if (auto character = GetOwnerAs<Character>(); character) {
      character->RemoveDefenseRule(defense);
    }

    fx->Eject();
    GetOwner()->RemoveNode(fx);
    Eject();
    return;
  }

 if (GetOwner()->GetTile() == nullptr) {
   fx->Hide();
   return;
 }

 if (GetOwner()->WillRemoveLater()) {
   timer = 0;
   fx->Eject();
   GetOwner()->RemoveNode(fx);
   Eject();
 }

 // Update the visual fx
 fx->timer = timer;
 fx->currHP = currHP;
}

void Aura::Inject(BattleSceneBase& bs)
{
  bs.Inject(this);
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

void Aura::TakeDamage(int damage)
{
  if (type >= Aura::Type::BARRIER_10) {
    health = health - damage;
  }
  else if (health <= damage) {
    health = 0;
  }

  if (health <= 0) {
    health = 0;
  }
}

Aura::~Aura()
{
  delete defense;
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
}

void Aura::VisualFX::draw(sf::RenderTarget& target, sf::RenderStates states) const {
  auto this_states = states;
  this_states.transform *= getTransform();

  UIComponent::draw(target, this_states);

  // Only draw HP font for Barriers. Auras are hidden.
  if (type <= Type::AURA_1000) return;

  // 0 - 5 are on first row
  // Glyphs are 8x15 
  // 2nd row starts +1 pixel down

  if (currHP > 0) {
    int size = (int)(std::to_string(currHP).size());
    int hp = currHP;
    float offsetx = -(((size) * 8.0f) / 2.0f) * font.getScale().x;
    int index = 0;
    while (index < size) {
      const char c = std::to_string(currHP)[index];
      int number = std::atoi(&c);
      int rowstart = 0;

      if (number > 4) {
        rowstart = 16;
      }

      int col = 8 * (number % 5);

      font.setTextureRect(sf::IntRect(col, rowstart, 8, 15));
      font.setPosition(sf::Vector2f(offsetx, 35.0f));


      float fontScaleX = font.getScale().x;

      // this is flipped, but we do not want to flip fonts
      if (this_states.transform.getMatrix()[0] < 0.0f && fontScaleX > 0.f) {
        font.setScale(-fontScaleX, font.getScale().y);
      }

      auto font_states = this_states;
      font_states.transform = font.getTransform();

      target.draw(font, font_states);

      offsetx += 8.0f * font.getScale().x;

      index++;
    }
  }
}

void Aura::VisualFX::Inject(BattleSceneBase& bs) {
  bs.Inject((Component*)this);
  AUDIO.Play(AudioType::APPEAR);
}

void Aura::VisualFX::OnUpdate(float _elapsed)
{
  timer += _elapsed;

  if (currHP == 0) {
    auraSprite.setColor(sf::Color(255, 255, 255, 50));
  }
  else {
    // flicker
    if (int(timer * 15000) % 3 == 0) {
      Hide();
    }
    else {
      Reveal();
    }
  }

  animation.Update(_elapsed, aura->getSprite());
}