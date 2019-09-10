#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnSpell.h"
#include "bnAura.h"
#include "bnLayered.h"
#include "bnDefenseAura.h"
#include "bnBattleScene.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/spells/auras.animation"

Aura::Aura(Aura::Type type, Character* owner) : type(type), SceneNode(), Component(owner), privOwner(owner)
{
  this->timer = 50; // seconds
  
  auraSprite.setTexture(*TEXTURES.GetTexture(TextureType::SPELL_AURA));
  aura = new SpriteSceneNode(auraSprite);
  
  // owner draws -> aura component draws -> aura sprite anim draws
  owner->RegisterComponent(this);
  owner->AddNode(this);
  this->AddNode(aura);

  persist = false;

  font.setTexture(LOAD_TEXTURE(AURA_NUMSET));
  font.setScale(1.f, 1.f);
  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  isOver = false;

  switch (type) {
  case Aura::Type::AURA_100:
    animation.SetAnimation("AURA_100");
    this->health = 100;
    break;
  case Aura::Type::AURA_200:
    animation.SetAnimation("AURA_200");
    this->health = 200;
    break;
  case Aura::Type::AURA_1000:
    animation.SetAnimation("AURA_1000");
    this->health = 1000;;
    break;
  case Aura::Type::BARRIER_100:
    animation.SetAnimation("BARRIER_100");
    this->health = 100;
    break;
  case Aura::Type::BARRIER_200:
    animation.SetAnimation("BARRIER_200");
    this->health = 200;
    break;
  case Aura::Type::BARRIER_500:
    animation.SetAnimation("BARRIER_500");
    this->health = 500;
    break;
  }
  
  currHP = health;

  DefenseAura::Callback onHit = [this](Spell* in, Character* owner) {
	  this->TakeDamage(in->GetHitboxProperties().damage);
  };
  
  this->defense = new DefenseAura(onHit);
  
  owner->AddDefenseRule(defense);

  animation << Animator::Mode::Loop;
  animation.Update(0, *aura);
}

void Aura::Inject(BattleScene& bs) {
  this->bs = &bs;
  this->bs->Inject((Component*)this);
  GetOwner()->FreeComponentByID(this->GetID());
}

void Aura::OnUpdate(float _elapsed) {
  if (this->bs == nullptr) {
    return;
  }

  currHP = health;
  
  if(this->bs->IsBattleActive() && (!persist || isOver)) {
    timer -= _elapsed;
  }

  if (!isOver) {
    if (health == 0 || timer <= 0.0) {
      isOver = true;
      timer = 2;
      this->auraSprite.setColor(sf::Color(255, 255, 255, 230));
    }

    this->Reveal(); // always show regardless of owner
  }
  else if (timer <= 0.0) {
    this->privOwner->RemoveDefenseRule(this->defense);
    this->RemoveNode(aura);
    this->privOwner->RemoveNode(this);
    this->bs->Eject(this);
    delete this;
    return;
  }
  else {
    // flicker
    if (int(timer*15000) % 2 == 0) {
      this->Hide();
    }
    else {
      this->Reveal();
    }
  }

 if (this->privOwner->GetTile() == nullptr) {
   this->Hide();
   return;
 }

 animation.Update(_elapsed, *aura);
}

const Aura::Type Aura::GetAuraType()
{
  return type;
}
 
void Aura::Persist(bool enable) {
	this->persist = enable;
}

const bool Aura::IsPersistent() const {
	 return this->persist;
 }
  
const int Aura::GetHealth() const {
	return this->health;
}

void Aura::TakeDamage(int damage)
{
  Logger::Log("Taking damage: " + std::to_string(damage) + " with type: " + std::to_string((int)type));

  if (type >= Aura::Type::BARRIER_100) {
    health = health - damage;
  }
  else if (health <= damage) {
    health = 0;
  }

  if (health <= 0) {
    health = 0;
  }
}

void Aura::draw(sf::RenderTarget& target, sf::RenderStates states) const {  
  auto this_states = states;
  this_states.transform *= this->getTransform();
  this_states.shader = nullptr; // we don't want to apply effects from the owner to this component
  SceneNode::draw(target, this_states);

  // 0 - 5 are on first row
  // Glyphs are 8x15 
  // 2nd row starts +1 pixel down

  if (currHP > 0) {
    int size = (int)(std::to_string(currHP).size());
    int hp = currHP;
    float offsetx = -(((size)*8.0f) / 2.0f)*font.getScale().x;
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
      font.setPosition(sf::Vector2f(offsetx, -50.0f));

      target.draw(font, this_states);
      //ENGINE.Draw(font);

      offsetx += 8.0f*font.getScale().x;
      index++;
    }
  }
}

Aura::~Aura()
{
  delete this->aura;
  delete this->defense;
}
