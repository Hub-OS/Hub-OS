#include "bnPlayer.h"
#include "bnExplodeState.h"
#include "bnField.h"
#include "bnBuster.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnEngine.h"
#include "bnLogger.h"

#define RESOURCE_PATH "resources/navis/megaman/megaman.animation"

Player::Player(void)
  : health(150),
  state(PLAYER_IDLE),
  chargeComponent(this),
  animationComponent(this),
  AI<Player>(this),
  Character(Rank::_1)
{
  this->ChangeState<PlayerIdleState>();

  name = "Megaman";
  SetLayer(0);
  team = Team::RED;

  moveCount = hitCount = 0;

  //Animation
  animationProgress = 0.0f;
  setScale(2.0f, 2.0f);

  healthUI = new PlayerHealthUI(this);

  //Components setup and load
  chargeComponent.load();

  animationComponent.Setup(RESOURCE_PATH);
  animationComponent.Load();

  textureType = TextureType::NAVI_MEGAMAN_ATLAS;
  setTexture(*TEXTURES.GetTexture(textureType));

  previous = nullptr;

  moveCount = 0;

  invincibilityCooldown = 0;
  alpha = 255;

  cloakTimeSecs = 0;
}

Player::~Player(void) {
  delete healthUI;
}

void Player::Update(float _elapsed) {
  //Update UI of player's health (top left corner)
  healthUI->Update();

  animationComponent.Update(_elapsed);

  if (_elapsed <= 0)
    return;

  if (tile != nullptr) {
    setPosition(tileOffset.x + tile->getPosition().x + (tile->GetWidth() / 2.0f), tileOffset.y + tile->getPosition().y + (tile->GetHeight() / 2.0f));
  }

  // Explode if health depleted
  if (GetHealth() <= 0) {
    this->ChangeState<ExplodeState<Player>>(10, 0.65);
    AI<Player>::Update(_elapsed);
    return;
  }

  // TODO: Get rid of this. Put this type of behavior in a component
  if (cloakTimer.getElapsedTime() > sf::seconds((float)cloakTimeSecs) && cloakTimeSecs != 0) {
    this->SetPassthrough(false);
    this->SetAlpha(255);
    cloakTimeSecs = 0;
  }

  if (invincibilityCooldown > 0) {
    if ((((int)(invincibilityCooldown * 15))) % 2 == 0) {
      this->setColor(sf::Color(255,255,255,0));
    }
    else {
          this->setColor(sf::Color(255, 255, 255, alpha));
    }

    invincibilityCooldown -= _elapsed;
  }
  else {
    this->setColor(sf::Color(255, 255, 255, alpha));
  }

  AI<Player>::Update(_elapsed);

  //Components updates
  chargeComponent.update(_elapsed);

  Character::Update(_elapsed);
}

void Player::Attack(float _charge) {
  if (tile->GetX() <= static_cast<int>(field->GetWidth())) {
    Spell* spell = new Buster(field, team, chargeComponent.IsFullyCharged());
    spell->SetDirection(Direction::RIGHT);
    field->AddEntity(spell, tile->GetX(), tile->GetY());
  }
}

vector<Drawable*> Player::GetMiscComponents() {
  vector<Drawable*> drawables = vector<Drawable*>();
  Drawable* component;
  while (healthUI->GetNextComponent(component)) {
    drawables.push_back(component);
  }
  drawables.push_back(&chargeComponent.GetSprite());

  return drawables;
}

void Player::SetHealth(int _health) {
  health = _health;

  if (health < 0) health = 0;

  healthUI->Update();
}

int Player::GetHealth() const {
  return health;
}

const bool Player::Hit(int _damage) {
  // return false;

  if (this->IsPassthrough() || invincibilityCooldown > 0) return false;

  bool result = true;

  if (health - _damage < 0) {
    health = 0;
  }
  else {
    health -= _damage;
    hitCount++;
    this->ChangeState<PlayerHitState, float>({ 600.0f });
  }

  return result;
}

int Player::GetMoveCount() const
{
  return moveCount;
}

int Player::GetHitCount() const
{
  return hitCount;
}

PlayerHealthUI* Player::GetHealthUI() const {
  return healthUI;
}

AnimationComponent& Player::GetAnimationComponent() {
  return animationComponent;
}

void Player::SetCharging(bool state)
{
  chargeComponent.SetCharging(state);
}

void Player::SetAlpha(int value)
{
  alpha = value;
  this->setColor(sf::Color(255, 255, 255, alpha));
}

void Player::SetCloakTimer(int seconds)
{
  SetPassthrough(true);
  SetAlpha(255 / 2);
  cloakTimer.restart();
  cloakTimeSecs = seconds;
}

void Player::SetAnimation(string _state, std::function<void()> onFinish) {
  state = _state;

  if (state == PLAYER_IDLE) {
    int playback = Animate::Mode::Loop;
    animationComponent.SetAnimation(_state, Animate::Mode(playback), onFinish);
  }
  else {
    animationComponent.SetAnimation(_state, onFinish);
  }
}
