#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
<<<<<<< HEAD
#include "bnAura.h"
#include "bnAuraHealthUI.h"
=======
#include "bnSpell.h"
#include "bnAura.h"
#include "bnLayered.h"
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
#include "bnDefenseAura.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/spells/auras.animation"

<<<<<<< HEAD
Aura::Aura(Aura::Type type, Character* owner) : type(type), Character(), Component(owner)
{
  SetLayer(1);
  this->timer = 50; // seconds
  this->setTexture(*TEXTURES.GetTexture(TextureType::SPELL_AURA));
  this->setScale(2.f, 2.f);
  this->SetTeam(owner->GetTeam());
  this->SetFloatShoe(true);
  this->ShareTileSpace(true);

  healthUI = new AuraHealthUI(this);
  this->RegisterComponent(healthUI);

  aura = (sf::Sprite)*this;
=======
Aura::Aura(Aura::Type type, Character* owner) : type(type), SceneNode(), Component(owner)
{
  this->timer = 50; // seconds
  
  auraSprite.setTexture(*TEXTURES.GetTexture(TextureType::SPELL_AURA));
  aura = new SpriteSceneNode(auraSprite);
  
  // owner draws -> aura component draws -> aura sprite anim draws
  owner->AddNode(this);
  this->AddNode(aura);
  
  font.setTexture(LOAD_TEXTURE(AURA_NUMSET));
  font.setScale(1.f, 1.f);
>>>>>>> b486e21e11627262088deae73097eaa7af56791c

  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  switch (type) {
  case Aura::Type::AURA_100:
    animation.SetAnimation("AURA_100");
<<<<<<< HEAD
    this->SetHealth(100);
    break;
  case Aura::Type::AURA_200:
    animation.SetAnimation("AURA_200");
    this->SetHealth(200);
    break;
  case Aura::Type::AURA_1000:
    animation.SetAnimation("AURA_1000");
    this->SetHealth(1000);
    break;
  case Aura::Type::BARRIER_100:
    animation.SetAnimation("BARRIER_100");
    this->SetHealth(100);
    break;
  case Aura::Type::BARRIER_200:
    animation.SetAnimation("BARRIER_200");
    this->SetHealth(200);
    break;
  case Aura::Type::BARRIER_500:
    animation.SetAnimation("BARRIER_500");
    this->SetHealth(500);
    break;
  }

  this->defense = new DefenseAura();
  owner->AddDefenseRule(defense);

  animation << Animate::Mode::Loop;
  animation.Update(0, *this);
=======
    this->health = 100;
    break;
  case Aura::Type::AURA_200:
    animation.SetAnimation("AURA_200");
    this->health = 200;
    break;
  case Aura::Type::AURA_1000:
    animation.SetAnimation("AURA_1000");
    this->health = 1000;
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
  
  currHP = owner->GetHealth();

  DefenseAura::Callback onHit = [this](Spell* in, Character* owner) {
	  this->TakeDamage(in->GetHitboxProperties().damage);
  };
  
  this->defense = new DefenseAura(onHit);
  
  owner->AddDefenseRule(defense);

  animation << Animate::Mode::Loop;
  animation.Update(0, *aura);
  
  persist = false;
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
}

void Aura::Inject(BattleScene& bs) {

}

void Aura::Update(float _elapsed) {
<<<<<<< HEAD
  timer -= _elapsed;

  this->SlideToTile(false);

  if (GetHealth() == 0 || timer <= 0.0) {
    this->GetOwnerAs<Character>()->RemoveDefenseRule(this->defense);
    this->GetOwner()->FreeComponentByID(this->Component::GetID());
    this->FreeAllComponents();
    this->TryDelete();
    Entity::Update(_elapsed);

    return;
  }

 if (this->tile != this->GetOwner()->GetTile()) {
   if (this->tile) {
     this->tile->RemoveEntityByID(this->Entity::GetID());
   }

   this->SetTile(this->GetOwner()->GetTile());

   if (tile) {
     this->tile->AddEntity(*this);
   }
 }

 if (this->GetOwner()->GetTile() == nullptr) {
   this->Hide();
   healthUI->Hide();
=======
  currHP = health;
  
  if(!persist) {
    timer -= _elapsed;
  }

  if (health == 0 || timer <= 0.0) {
    this->GetOwnerAs<Character>()->RemoveDefenseRule(this->defense);
    this->RemoveNode(aura);
    this->GetOwner()->RemoveNode(this);
    this->GetOwner()->FreeComponentByID(this->Component::GetID());
    delete this;
    return;
  }

 if (this->GetOwner()->GetTile() == nullptr) {
   this->Hide();
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
   return;
 }
 else {
   this->Reveal();
<<<<<<< HEAD
   healthUI->Reveal();

   this->setPosition(this->tile->getPosition());
 }


 animation.Update(_elapsed, *this);
 Entity::Update(_elapsed);

 if (IsDeleted()) {
   delete this;
 }
=======
   //this->setPosition(GetOwner()->getPosition());
   //aura.setPosition(this->getPosition());
 }

 animation.Update(_elapsed, *aura);
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
}

const Aura::Type Aura::GetAuraType()
{
  return type;
}
<<<<<<< HEAD

const bool Aura::Hit(Hit::Properties props)
{
  int health = this->GetHealth();

  if (type >= Aura::Type::BARRIER_100) {
   health = health - props.damage;
  }
  else if (health <= props.damage) {
=======
 
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

  if (type >= Aura::Type::BARRIER_100) {
    health = health - damage;
  }
  else if (health <= damage) {
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
    health = 0;
  }

  if (health <= 0) {
    health = 0;
  }
<<<<<<< HEAD

  this->SetHealth(health);

  return true;
=======
}

void Aura::draw(sf::RenderTarget& target, sf::RenderStates states) const {  
  auto this_states = states;
  this_states.transform *= this->getTransform();

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
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
}

Aura::~Aura()
{
<<<<<<< HEAD
  delete healthUI;
  delete this->defense;
}
=======
  delete this->aura;
  delete this->defense;
}
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
