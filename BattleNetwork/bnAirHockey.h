#include "bnSpell.h"

class AirHockey : public Spell {
  Team lastTileTeam{};
  Direction dir{};
  int damage{};
  int moveCount{};
  bool reflecting{ false };
  std::shared_ptr<sf::SoundBuffer> launchSfx, bounceSfx;

  const Direction Bounce(const Direction& dir, Battle::Tile& next);
public:
  AirHockey(Field* field, Team team, int damage, int moveCount);
  ~AirHockey();

  void OnUpdate(double _elapsed) override;
  void Attack(Character* _entity) override;
  bool CanMoveTo(Battle::Tile* next) override;
  void OnSpawn(Battle::Tile& start) override;
  void OnDelete() override;
  void OnCollision(const Character*) override;
};