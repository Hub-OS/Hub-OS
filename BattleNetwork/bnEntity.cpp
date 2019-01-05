#include "bnEntity.h"
#include "bnTile.h"
#include "bnField.h"
#include "Swoosh\Game.h"

Entity::Entity()
  : tile(nullptr),
  next(nullptr),
  previous(nullptr),
  field(nullptr),
  previousDirection(Direction::NONE),
  direction(Direction::NONE),
  team(Team::UNKNOWN),
  deleted(false),
  passthrough(false),
  ownedByField(false),
  isSliding(false),
  element(Element::NONE),
  tileOffset(sf::Vector2f(0,0)) {
}

Entity::~Entity() {
}

void Entity::Update(float _elapsed) {
  if (_elapsed <= 0)
    return;

  if (isSliding && this->next) {
    
    sf::Vector2f pos = this->getPosition() + this->tileOffset;
    sf::Vector2f tar = this->next->getPosition();
    sf::Vector2f delta = swoosh::game::directionTo<float>(tar, pos);
    delta.x = delta.x * 200.0 * _elapsed;
    delta.y = delta.y * 200.0 *_elapsed;

    this->tileOffset += delta;

    auto x = tar.x - pos.x;
    auto y = tar.y - pos.y;

    x *= x; y *= y;

    std::cout << "delta.x, delta.y" << x << ", " << y << std::endl;

    if (x + y  < 10.0f)
    {
      Battle::Tile* prevTile = this->GetTile();
      this->tileOffset = sf::Vector2f(0, 0);
      this->AdoptNextTile();
      isSliding = this->GetTile() ? (this->GetTile()->GetState() == TileState::ICE) : false;

      if (isSliding) {
        std::cout << "we are sliding" << std::endl;;

        if(this->GetTile()->GetX() < prevTile->GetX()) {
          this->next = this->GetField()->GetAt(this->GetTile()->GetX() - 1, this->GetTile()->GetY());
        }
        else if (this->GetTile()->GetX() > prevTile->GetX()) {
          this->next = this->GetField()->GetAt(this->GetTile()->GetX() + 1, this->GetTile()->GetY());
        }
        else if (this->GetTile()->GetY() > prevTile->GetY()) {
          this->next = this->GetField()->GetAt(this->GetTile()->GetX(), this->GetTile()->GetY() + 1);
        }
        else if (this->GetTile()->GetY() < prevTile->GetY()) {
          this->next = this->GetField()->GetAt(this->GetTile()->GetX(), this->GetTile()->GetY() - 1);
        }

        if (!(this->next && this->next->GetTeam() == this->GetTeam())) {
          // Conditions not met
          std::cout << "Conditions not met" << std::endl;
          isSliding = false;
          this->next = nullptr;
        }
      }
    }
  }

  if (IsDeleted()) {
    field->RemoveEntity(this);
  }
}

vector<Drawable*> Entity::GetMiscComponents() {
  assert(false && "GetMiscComponents shouldn't be called directly from Entity");
  return vector<Drawable*>();
}

const float Entity::GetHitHeight() const {
  //assert(false && "GetHitHeight shouldn't be called directly from Entity");
  return 0;
}

const bool Entity::Hit(int damage) {
  return false;
}

TextureType Entity::GetTextureType() {
  assert(false && "GetGetTextureType shouldn't be called directly from Entity");
  return (TextureType)0;
}

bool Entity::Teammate(Team _team) {
  return team == _team;
}

void Entity::SetTile(Battle::Tile* _tile) {
  tile = _tile;
}

Battle::Tile* Entity::GetTile() const {
  return tile;
}

void Entity::SlideToTile(bool enabled)
{
  isSliding = enabled;
}

void Entity::SetField(Field* _field) {
  field = _field;
}

Field* Entity::GetField() const {
  return field;
}

Team Entity::GetTeam() const {
  return team;
}
void Entity::SetTeam(Team _team) {
  team = _team;
}

void Entity::SetPassthrough(bool state)
{
  passthrough = state;
}

bool Entity::IsPassthrough()
{
  return passthrough;
}

void Entity::SetFloatShoe(bool state)
{
  floatShoe = state;
}

bool Entity::HasFloatShoe()
{
  return floatShoe;
}

Direction Entity::GetPreviousDirection()
{
  return previousDirection;
}

void Entity::Delete()
{
  deleted = true;
}

bool Entity::IsDeleted() const {
  return deleted;
}

void Entity::SetElement(Element _elem)
{
  this->element = _elem;
}

const Element Entity::GetElement() const
{
  return element;
}

void Entity::AdoptNextTile()
{
  if (next == nullptr) return;

  SetTile(next);
  tile->AddEntity(this);

  if (previous != nullptr) {
    previous->RemoveEntity(this);
    previous = nullptr;
  }

  next = nullptr;

  moveCount++;
}
