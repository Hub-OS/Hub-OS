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

Aura::Aura(Aura::Type type, Character* owner) : type(type), SceneNode(), Component(owner), privOwner(owner), bs(nullptr)
{
  timer = 50; // seconds
  
  auraSprite.setTexture(*TEXTURES.GetTexture(TextureType::SPELL_AURA));
  aura = new SpriteProxyNode(auraSprite);

  // owner draws -> aura component draws -> aura sprite anim draws
  owner->RegisterComponent(this);
  owner->AddNode(this);

  AddNode(aura);

  // Note: need to get rid of artificial scaling by 2. Makes the math awful. No need for it.
  setPosition(0, -owner->GetHeight() / 2.0f / 2.0f); // divide twice. 1 -> real screen coord 2 -> half of real height

  persist = false;

  fontTextureRef = LOAD_TEXTURE(AURA_NUMSET);
  font.setTexture(*fontTextureRef);
  font.setScale(1.f, 1.f);
  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  isOver = false;

  switch (type) {
  case Aura::Type::AURA_100:
    animation.SetAnimation("AURA_100");
    health = 100;
    break;
  case Aura::Type::AURA_200:
    animation.SetAnimation("AURA_200");
    health = 200;
    break;
  case Aura::Type::AURA_1000:
    animation.SetAnimation("AURA_1000");
    health = 1000;;
    break;
  case Aura::Type::BARRIER_10:
    animation.SetAnimation("BARRIER_10");
    health = 100;
    break;
  case Aura::Type::BARRIER_200:
    animation.SetAnimation("BARRIER_200");
    health = 200;
    break;
  case Aura::Type::BARRIER_500:
    animation.SetAnimation("BARRIER_500");
    health = 500;
    break;
  }
  
  currHP = health;

  DefenseAura::Callback onHit = [this](Spell& in, Character& owner) {
    TakeDamage(in.GetHitboxProperties().damage);
  };
  
  defense = new DefenseAura(onHit);
  
  owner->AddDefenseRule(defense);

  animation << Animator::Mode::Loop;
  animation.Update(0, aura->getSprite());
}

void Aura::Inject(BattleSceneBase& bs) {
  bs.Inject((Component*)this);
  this->bs = &bs;
}

void Aura::OnUpdate(float _elapsed) {
  if (bs == nullptr) {
    return;
  }

  // If the aura has been replaced by another defense rule, remove all
  // associated components 
  if (defense->IsReplaced()) {
    RemoveNode(aura);
    privOwner->RemoveNode(this);
    bs->Eject(GetID());
    return;
  }

  currHP = health;
  
  if(!persist || isOver) {
    timer -= _elapsed;
  }

  if (!isOver) {
    if (health == 0 || timer <= 0.0) {
      isOver = true;
      timer = 2;
      auraSprite.setColor(sf::Color(255, 255, 255, 50));
    }

    Reveal(); // always show regardless of owner
  }
  else if (timer <= 0.0) {
    privOwner->RemoveDefenseRule(defense);
    privOwner->RemoveNode(this);
    bs->Eject(GetID());
    return;
  }
  else {
    // flicker
    if (int(timer*15000) % 2 == 0) {
      Hide();
    }
    else {
      Reveal();
    }
  }

 if (privOwner->GetTile() == nullptr) {
   Hide();
   return;
 }

 animation.Update(_elapsed, aura->getSprite());

 if (privOwner->WillRemoveLater()) {
   timer = 0;
   bs->Eject(GetID());
 }
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

void Aura::draw(sf::RenderTarget& target, sf::RenderStates states) const {  
  auto this_states = states;
  this_states.transform *= getTransform();

  this_states.shader = nullptr; // we don't want to apply effects from the owner to this component
  SceneNode::draw(target, this_states);

  // Only draw HP font for Barriers. Auras are hidden.
  if (type <= Type::AURA_1000) return;

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
      font.setPosition(sf::Vector2f(offsetx, 35.0f));


      // this is flipped, but we do not want to flip fonts
      bool flipped = true;
      float fontScaleX = font.getScale().x;
      if (this_states.transform.getMatrix()[0] < 0.0f && fontScaleX > 0.f) {
        font.setScale(-fontScaleX, font.getScale().y);
      }

      target.draw(font, this_states);

      offsetx += 8.0f*font.getScale().x;

      index++;
    }
  }
}

Aura::~Aura()
{
  delete aura;
  delete defense;
}
