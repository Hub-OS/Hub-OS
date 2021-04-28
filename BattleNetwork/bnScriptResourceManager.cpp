#ifdef BN_MOD_SUPPORT
#include "bnScriptResourceManager.h"
#include "bnAnimator.h"
#include "bnAudioResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnResourceHandle.h"
#include "bnEntity.h"
#include "bnSpell.h"
#include "bnElements.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnHitbox.h"
#include "bnSharedHitbox.h"
#include "bnDefenseNodrag.h"
#include "bnDefenseVirusBody.h"
#include "bnParticlePoof.h"
#include "bnParticleImpact.h"

#include "bnNaviRegistration.h"
#include "bnMobRegistration.h"
#include "bindings/bnScriptedCardAction.h"
#include "bindings/bnScriptedCharacter.h"
#include "bindings/bnScriptedSpell.h"
#include "bindings/bnScriptedObstacle.h"
#include "bindings/bnScriptedPlayer.h"
#include "bindings/bnScriptedDefenseRule.h"
#include "bindings/bnScriptedMob.h"

// Useful prefabs to use in scripts...
#include "bnExplosion.h"

// temporary proof of concept includes...
#include "bnBusterCardAction.h"
#include "bnSwordCardAction.h"
#include "bnBombCardAction.h"
#include "bnFireBurnCardAction.h"
#include "bnCannonCardAction.h"

void ScriptResourceManager::ConfigureEnvironment(sol::state& state) {
  state.open_libraries(sol::lib::base, sol::lib::math);

  // create_table returns non reference to global table? internal tracking?
  sol::table battle_namespace = state.create_table("Battle");
  sol::table overworld_namespace = state.create_table("Overworld");
  sol::table engine_namespace = state.create_table("Engine");

  // global namespace
  const auto& color_record = state.new_usertype<sf::Color>("Color",
    sol::constructors<sf::Color(sf::Uint8, sf::Uint8, sf::Uint8, sf::Uint8)>()
  );

  const auto& vector_record = state.new_usertype<sf::Vector2f>("Vector2",
    "x", &sf::Vector2f::x,
    "y", &sf::Vector2f::y
  );

  const auto& animation_record = engine_namespace.new_usertype<Animation>("Animation",
    sol::constructors<Animation(const std::string&), Animation(const Animation&)>(),
    "Load", &Animation::Load,
    "Update", &Animation::Update,
    "Refresh", &Animation::Refresh,
    "SetState", &Animation::SetAnimation,
    "CopyFrom", &Animation::CopyFrom,
    "State", &Animation::GetAnimationString,
    "Point", &Animation::GetPoint,
    "SetPlayback", sol::resolve<Animation&(char)>(&Animation::operator<<),
    "OnComplete", sol::resolve<void(const FrameCallback&)>(&Animation::operator<<),
    "OnFrame", &Animation::AddCallback,
    "OnInterrupt", &Animation::SetInterruptCallback
  );

  const auto& node_record = engine_namespace.new_usertype<SpriteProxyNode>("SpriteNode",
    sol::constructors<SpriteProxyNode()>(),
    "SetTexture", &SpriteProxyNode::setTexture,
    "SetLayer", &SpriteProxyNode::SetLayer,
    "Show", &SpriteProxyNode::Reveal,
    "Hide", &SpriteProxyNode::Hide,
    "SetPosition", sol::resolve<void(float, float)>(&SpriteProxyNode::setPosition),
    "GetPosition", &SpriteProxyNode::getPosition,
    "Sprite", &SpriteProxyNode::getSprite,
    "EnableParentShader", &SpriteProxyNode::EnableParentShader,
    sol::base_classes, sol::bases<SceneNode>()
  );

  const auto& defense_frame_state_judge_record = battle_namespace.new_usertype<DefenseFrameStateJudge>("DefenseFrameStateJudge",
    "BlockDamage", &DefenseFrameStateJudge::BlockDamage,
    "BlockImpact", &DefenseFrameStateJudge::BlockImpact,
    "IsDamageBlocked", &DefenseFrameStateJudge::IsDamageBlocked,
    "IsImpactBlocked", &DefenseFrameStateJudge::IsImpactBlocked,
    /*"AddTrigger", &DefenseFrameStateJudge::AddTrigger,*/
    "SignalDefenseWasPierced", &DefenseFrameStateJudge::SignalDefenseWasPierced
    );

  const auto& defense_rule_record = battle_namespace.new_usertype<ScriptedDefenseRule>("DefenseRule",
    sol::factories([](int priority, const DefenseOrder& order) -> std::unique_ptr<ScriptedDefenseRule> {
      return std::make_unique<ScriptedDefenseRule>(Priority(priority), order);
    }),
    "IsReplaced", &ScriptedDefenseRule::IsReplaced,
    "canBlockFunc", &ScriptedDefenseRule::canBlockCallback,
    "filterStatusesFunc", &ScriptedDefenseRule::filterStatusesCallback,
    sol::base_classes, sol::bases<DefenseRule>()
    );

  const auto& defense_rule_nodrag = battle_namespace.new_usertype<DefenseNodrag>("DefenseNoDrag",
    sol::factories([] {
      return std::make_unique<DefenseNodrag>();
    }),
    sol::base_classes, sol::bases<DefenseRule>()
  );

  const auto& defense_rule_virus_body = battle_namespace.new_usertype<DefenseVirusBody>("DefenseVirusBody",
    sol::factories([] {
      return std::make_unique<DefenseVirusBody>();
    }),
    sol::base_classes, sol::bases<DefenseRule>()
  );

  const auto& tile_record = battle_namespace.new_usertype<Battle::Tile>("Tile",
    "X", &Battle::Tile::GetX,
    "Y", &Battle::Tile::GetY,
    "Width", &Battle::Tile::GetWidth,
    "Height", &Battle::Tile::GetHeight,
    "GetState", &Battle::Tile::GetState,
    "SetState", &Battle::Tile::SetState,
    "IsEdge", &Battle::Tile::IsEdgeTile,
    "IsCracked", &Battle::Tile::IsCracked,
    "IsHole", &Battle::Tile::IsHole,
    "IsWalkable", &Battle::Tile::IsWalkable,
    "IsReserved", &Battle::Tile::IsReservedByCharacter,
    "Team", &Battle::Tile::GetTeam,
    "AttackEntities", &Battle::Tile::AffectEntities
  );

  const auto& field_record = battle_namespace.new_usertype<Field>("Field",
    "TileAt", &Field::GetAt,
    "Width", &Field::GetWidth,
    "Height", &Field::GetHeight,
    "Spawn", sol::overload(
      sol::resolve<Field::AddEntityStatus(Spell&, int, int)>(&Field::AddEntity),
      sol::resolve<Field::AddEntityStatus(Artifact&, int, int)>(&Field::AddEntity),
      sol::resolve<Field::AddEntityStatus(std::unique_ptr<ScriptedSpell>&, int, int)>(&Field::AddEntity),
      sol::resolve<Field::AddEntityStatus(std::unique_ptr<ScriptedObstacle>&, int, int)>(&Field::AddEntity)
    )
  );

  const auto& player_record = battle_namespace.new_usertype<Player>("Player",
    sol::base_classes, sol::bases<Character>()
  );

  const auto& explosion_record = battle_namespace.new_usertype<Explosion>("Explosion",
    sol::factories([](int count, double speed) {
      return new Explosion(count, speed);
    }),
    sol::base_classes, sol::bases<Artifact>()
  );

  // auto& frame_time_record = state.new_usertype<frame_time_t>("Frame");
  const auto& move_event_record = state.new_usertype<MoveEvent>("MoveEvent");

  const auto& scriptedspell_record = battle_namespace.new_usertype<ScriptedSpell>("Spell",
    sol::factories([](Team team) -> std::unique_ptr<ScriptedSpell> {
      return std::make_unique<ScriptedSpell>(team);
    }),
    sol::meta_function::index,
    &dynamic_object::dynamic_get,
    sol::meta_function::new_index,
    &dynamic_object::dynamic_set,
    sol::meta_function::length,
    [](dynamic_object& d) { return d.entries.size(); },
    "GetID", &ScriptedSpell::GetID,
    "SetHeight", &ScriptedSpell::SetHeight,
    "SetTexture", &ScriptedSpell::setTexture,
    "SetLayer", &ScriptedSpell::SetLayer,
    "GetAnimation", &ScriptedSpell::GetAnimationObject,
    "GetTile", &ScriptedSpell::GetTile,
    "Tile", &ScriptedSpell::GetCurrentTile,
    "Field", &ScriptedSpell::GetField,
    "Slide", &ScriptedSpell::Slide,
    "Jump", &ScriptedSpell::Jump,
    "Teleport", & ScriptedSpell::Teleport,
    "RawMoveEvent", &ScriptedSpell::RawMoveEvent,
    "IsSliding", &ScriptedSpell::IsSliding,
    "IsJumping", &ScriptedSpell::IsJumping,
    "IsTeleporting", &ScriptedSpell::IsTeleporting,
    "IsMoving", &ScriptedSpell::IsMoving,
    "SetPosition", &ScriptedSpell::SetTileOffset,
    "GetPosition", &ScriptedSpell::GetTileOffset,
    "ShowShadow", &ScriptedSpell::ShowShadow,
    "Teammate", &ScriptedSpell::Teammate,
    "AddNode", &ScriptedSpell::AddNode,
    "Delete", &ScriptedSpell::Delete,
    "HighlightTile", &ScriptedSpell::HighlightTile,
    "GetHitProps", &ScriptedSpell::GetHitboxProperties,
    "SetHitProps", &ScriptedSpell::SetHitboxProperties,
    "GetTileOffset", &ScriptedSpell::GetTileOffset,
    "ShakeCamera", &ScriptedSpell::ShakeCamera,
    "Remove", &ScriptedSpell::Remove,
    "Team", &ScriptedSpell::GetTeam,
    "attackFunc", &ScriptedSpell::attackCallback,
    "deleteFunc", &ScriptedSpell::deleteCallback,
    "updateFunc", &ScriptedSpell::updateCallback,
    "canMoveToFunc", &ScriptedSpell::canMoveToCallback,
    "onSpawnFunc", &ScriptedSpell::spawnCallback,
    sol::base_classes, sol::bases<Spell>()
  );

  const auto& scriptedobstacle_record = battle_namespace.new_usertype<ScriptedObstacle>("Obstacle",
    sol::factories([](Team team) -> std::unique_ptr<ScriptedObstacle> {
      return std::make_unique<ScriptedObstacle>(team);
    }),
    sol::meta_function::index,
    &dynamic_object::dynamic_get,
    sol::meta_function::new_index,
    &dynamic_object::dynamic_set,
    sol::meta_function::length,
    [](dynamic_object& d) { return d.entries.size(); },
    "GetID", &ScriptedObstacle::GetID,
    "SetHeight", &ScriptedObstacle::SetHeight,
    "SetTexture", &ScriptedObstacle::setTexture,
    "GetName", &ScriptedObstacle::GetName,
    "GetHealth", &ScriptedObstacle::GetHealth,
    "GetMaxHealth", &ScriptedObstacle::GetMaxHealth,
    "SetName", &ScriptedObstacle::SetName,
    "SetHealth", &ScriptedObstacle::SetHealth,
    "SetHeight", &ScriptedObstacle::SetHeight,
    "SetLayer", &ScriptedObstacle::SetLayer,
    "GetAnimation", &ScriptedObstacle::GetAnimationObject,
    "SetPosition", &ScriptedObstacle::SetTileOffset,
    "GetPosition", &ScriptedObstacle::GetTileOffset,
    "GetTile", &ScriptedObstacle::GetTile,
    "Tile", &ScriptedObstacle::GetCurrentTile,
    "Field", &ScriptedObstacle::GetField,
    "Slide", &ScriptedObstacle::Slide,
    "Jump", &ScriptedObstacle::Jump,
    "Teleport", &ScriptedObstacle::Teleport,
    "RawMoveEvent", &ScriptedObstacle::RawMoveEvent,
    "IsSliding", &ScriptedObstacle::IsSliding,
    "IsJumping", &ScriptedObstacle::IsJumping,
    "IsTeleporting", &ScriptedObstacle::IsTeleporting,
    "IsMoving", &ScriptedObstacle::IsMoving,
    "ShowShadow", &ScriptedObstacle::ShowShadow,
    "Teammate", &ScriptedObstacle::Teammate,
    "AddNode", &ScriptedObstacle::AddNode,
    "Delete", &ScriptedObstacle::Delete,
    "HighlightTile", &ScriptedObstacle::HighlightTile,
    "GetHitProps", &ScriptedObstacle::GetHitboxProperties,
    "SetHitProps", &ScriptedObstacle::SetHitboxProperties,
    "IgnoreCommonAggressor", &ScriptedObstacle::IgnoreCommonAggressor,
    "ShakeCamera", &ScriptedObstacle::ShakeCamera,
    "Remove", &ScriptedObstacle::Remove,
    "ShareTile", &ScriptedObstacle::ShareTileSpace,
    "AddDefenseRule", &ScriptedObstacle::AddDefenseRule,
    "Team", &ScriptedObstacle::GetTeam,
    "attackFunc", &ScriptedObstacle::attackCallback,
    "deleteFunc", &ScriptedObstacle::deleteCallback,
    "updateFunc", &ScriptedObstacle::updateCallback,
    "canMoveToFunc", &ScriptedObstacle::canMoveToCallback,
    "onSpawnFunc", &ScriptedObstacle::spawnCallback,
    sol::base_classes, sol::bases<Obstacle, Spell, Character>()
  );

  const auto& scriptedcharacter_record = battle_namespace.new_usertype<ScriptedCharacter>("Character",
    sol::meta_function::index,
    &dynamic_object::dynamic_get,
    sol::meta_function::new_index,
    &dynamic_object::dynamic_set,
    sol::meta_function::length,
    [](dynamic_object& d) { return d.entries.size(); },
    "GetName", &ScriptedCharacter::GetName,
    "GetID", &ScriptedCharacter::GetID,
    "GetRank", &ScriptedCharacter::GetRank,
    "SetRank", &ScriptedCharacter::SetRank,
    "GetHealth", &ScriptedCharacter::GetHealth,
    "GetMaxHealth", &ScriptedCharacter::GetMaxHealth,
    "SetPosition", &ScriptedCharacter::SetTileOffset,
    "GetPosition", &ScriptedCharacter::GetTileOffset,
    "SetName", &ScriptedCharacter::SetName,
    "SetHealth", &ScriptedCharacter::SetHealth,
    "SetHeight", &ScriptedCharacter::SetHeight,
    "SetTexture", &ScriptedCharacter::setTexture,
    "GetAnimation", &ScriptedCharacter::GetAnimationObject,
    "GetTile", &ScriptedCharacter::GetTile,
    "Tile", &ScriptedCharacter::GetCurrentTile,
    "Field", &ScriptedCharacter::GetField,
    "Target", &ScriptedCharacter::GetTarget,
    "Slide", &ScriptedCharacter::Slide,
    "Jump", &ScriptedCharacter::Jump,
    "Teleport", &ScriptedCharacter::Teleport,
    "RawMoveEvent", &ScriptedCharacter::RawMoveEvent,
    "IsSliding", &ScriptedCharacter::IsSliding,
    "IsJumping", &ScriptedCharacter::IsJumping,
    "IsTeleporting", &ScriptedCharacter::IsTeleporting,
    "IsMoving", &ScriptedCharacter::IsMoving,
    "ShareTile", &ScriptedCharacter::ShareTileSpace,
    "Teammate", &ScriptedCharacter::Teammate,
    "AddNode", &ScriptedCharacter::AddNode,
    "ShakeCamera", &ScriptedCharacter::ShakeCamera,
    "AddDefenseRule", &ScriptedCharacter::AddDefenseRule,
    "Team", &ScriptedCharacter::GetTeam,
    sol::base_classes, sol::bases<Character>()
  );

  const auto& scriptedplayer_record = battle_namespace.new_usertype<ScriptedPlayer>("Player",
    "GetName", &ScriptedPlayer::GetName,
    "GetID", &ScriptedPlayer::GetID,
    "GetHealth", &ScriptedPlayer::GetHealth,
    "GetMaxHealth", &ScriptedPlayer::GetMaxHealth,
    "SetName", &ScriptedPlayer::SetName,
    "SetHealth", &ScriptedPlayer::SetHealth,
    "SetTexture", &ScriptedPlayer::setTexture,
    "SetElement", &ScriptedPlayer::SetElement,
    "GetTile", &ScriptedPlayer::GetTile,
    "Field", &ScriptedPlayer::GetField,
    "Tile", &ScriptedPlayer::GetCurrentTile,
    "SetHeight",  &ScriptedPlayer::SetHeight,
    "SetFullyChargeColor", &ScriptedPlayer::SetFullyChargeColor,
    "SetChargePosition", &ScriptedPlayer::SetChargePosition,
    "GetAnimation", &ScriptedPlayer::GetAnimationObject,
    "SetAnimation", &ScriptedPlayer::SetAnimation,
    "SetFloatShoe", &ScriptedPlayer::SetFloatShoe,
    "SetAirShoe", &ScriptedPlayer::SetAirShoe,
    "SlideWhenMoving", &ScriptedPlayer::SlideWhenMoving,
    "Team", &ScriptedPlayer::GetTeam,
    sol::base_classes, sol::bases<Player>()
   );

  const auto& hitbox_record = battle_namespace.new_usertype<Hitbox>("Hitbox",
    sol::factories([](Team team) {
      return new Hitbox(team);
      }),
    "OnAttack", &Hitbox::AddCallback,
    "SetHitProps", &Hitbox::SetHitboxProperties,
    "GetHitProps", &Hitbox::GetHitboxProperties,
    sol::base_classes, sol::bases<Spell>()
  );

  const auto& shared_hitbox_record = battle_namespace.new_usertype<SharedHitbox>("SharedHitbox",
    sol::constructors<SharedHitbox(Spell*, float)>(),
    sol::base_classes, sol::bases<Spell>()
    );

  const auto& particle_poof = battle_namespace.new_usertype<ParticlePoof>("ParticlePoof",
    sol::constructors<ParticlePoof()>(),
    sol::base_classes, sol::bases<Artifact>()
    );

  const auto& particle_impact = battle_namespace.new_usertype<ParticleImpact>("ParticleImpact",
    sol::constructors<ParticleImpact(ParticleImpact::Type)>(),
    sol::base_classes, sol::bases<Artifact>()
  );

  const auto& busteraction_record = battle_namespace.new_usertype<BusterCardAction>("Buster",
    sol::factories([](Character& character, bool charged, int dmg) -> std::unique_ptr<CardAction> {
      return std::make_unique<BusterCardAction>(character, charged, dmg);
    }),
    sol::base_classes, sol::bases<CardAction>()
  );

  const auto& swordaction_record = battle_namespace.new_usertype<SwordCardAction>("Sword",
    sol::factories([](Character& character, int dmg) -> std::unique_ptr<CardAction> {
      return std::make_unique<SwordCardAction>(character, dmg);
    }),
    sol::base_classes, sol::bases<CardAction>()
  );

  const auto& bombaction_record = battle_namespace.new_usertype<BombCardAction>("Bomb",
    sol::factories([](Character& character, int dmg) -> std::unique_ptr<CardAction> {
      return std::make_unique<BombCardAction>(character, dmg);
    }),
    sol::base_classes, sol::bases<CardAction>()
  );

  const auto& fireburn_record = battle_namespace.new_usertype<FireBurnCardAction>("FireBurn",
    sol::factories([](Character& character, FireBurn::Type type, int dmg) -> std::unique_ptr<CardAction> {
      return std::make_unique<FireBurnCardAction>(character, type, dmg);
    }),
    sol::base_classes, sol::bases<CardAction>()
  );


  const auto& cannon_record = battle_namespace.new_usertype<CannonCardAction>("Cannon",
    sol::factories([](Character& character, CannonCardAction::Type type, int dmg) -> std::unique_ptr<CardAction> {
      return std::make_unique<CannonCardAction>(character, type, dmg);
      }),
    sol::base_classes, sol::bases<CardAction>()
  );


  const auto& textureresource_record = engine_namespace.new_usertype<TextureResourceManager>("TextureResourceManager",
    "LoadFile", &TextureResourceManager::LoadTextureFromFile
  );

  const auto& audioresource_record = engine_namespace.new_usertype<AudioResourceManager>("AudioResourceMananger",
    "LoadFile", &AudioResourceManager::LoadFromFile,
    "Stream", sol::resolve<int(std::string, bool)>(&AudioResourceManager::Stream)
  );

  const auto& shaderresource_record = engine_namespace.new_usertype<ShaderResourceManager>("ShaderResourceManager",
    "LoadFile", &ShaderResourceManager::LoadShaderFromFile
  );

  // make resource handle metatable
  const auto& resourcehandle_record = engine_namespace.new_usertype<ResourceHandle>("ResourceHandle",
    sol::constructors<ResourceHandle()>(),
    "Textures", sol::property(sol::resolve<TextureResourceManager& ()>(&ResourceHandle::Textures)),
    "Audio", sol::property(sol::resolve<AudioResourceManager& ()>(&ResourceHandle::Audio)),
    "Shaders", sol::property(sol::resolve<ShaderResourceManager& ()>(&ResourceHandle::Shaders))
  );

  // make loading resources easier
  // DOESNT WORK??
  /*state.script(
    "-- Shorthand load texture"
    "function LoadTexture(path)"
    "  return Engine.ResourceHandle.new().Textures:LoadFile(path)"
    "end"

    "-- Shorthand load audio"
    "function LoadAudio(path)"
    "  return Engine.ResourceHandle.new().Audio:LoadFile(path)"
    "end"

    "-- Shorthand load shader"
    "function LoadShader(path)"
    "  return Engine.ResourceHandle.new().Shaders:LoadFile(path)"
    "end"
  );*/

  // make meta object info metatable
  const auto& navimeta_table = engine_namespace.new_usertype<NaviRegistration::NaviMeta>("NaviMeta",
    "SetSpecialDescription", &NaviRegistration::NaviMeta::SetSpecialDescription,
    "SetAttack", &NaviRegistration::NaviMeta::SetAttack,
    "SetChargedAttack", &NaviRegistration::NaviMeta::SetChargedAttack,
    "SetSpeed", &NaviRegistration::NaviMeta::SetSpeed,
    "SetHP", &NaviRegistration::NaviMeta::SetHP,
    "SetIsSword", &NaviRegistration::NaviMeta::SetIsSword,
    "SetOverworldAnimationPath", &NaviRegistration::NaviMeta::SetOverworldAnimationPath,
    "SetOverworldTexturePath", &NaviRegistration::NaviMeta::SetOverworldTexturePath,
    "SetMugshotTexturePath", &NaviRegistration::NaviMeta::SetMugshotTexturePath,
    "SetMugshotAnimationPath", &NaviRegistration::NaviMeta::SetMugshotAnimationPath,
    "SetPreviewTexture", &NaviRegistration::NaviMeta::SetPreviewTexture,
    "SetIconTexture", &NaviRegistration::NaviMeta::SetIconTexture
  );

  const auto& mobmeta_table = engine_namespace.new_usertype<MobRegistration::MobMeta>("MobInfo",
    "SetDescription", &MobRegistration::MobMeta::SetDescription,
    "SetName", &MobRegistration::MobMeta::SetName,
    "SetPreviewTexturePath", &MobRegistration::MobMeta::SetPlaceholderTexturePath,
    "SetSpeed", &MobRegistration::MobMeta::SetSpeed,
    "SetAttack", &MobRegistration::MobMeta::SetAttack,
    "SetHP", &MobRegistration::MobMeta::SetHP
    );

  const auto& scriptedmob_table = engine_namespace.new_usertype<ScriptedMob>("Mob",
    "CreateSpawner", &ScriptedMob::CreateSpawner,
    "SetBackground", &ScriptedMob::SetBackground,
    "StreamMusic", &ScriptedMob::StreamMusic,
    "Field", &ScriptedMob::GetField
  );

  const auto& scriptedspawner_table = engine_namespace.new_usertype<ScriptedMob::ScriptedSpawner>("Spawner",
    "SpawnAt", &ScriptedMob::ScriptedSpawner::SpawnAt
  );

  engine_namespace.set_function("DefineCharacter",
    [this](const std::string& fqn, const std::string& path) {
      this->DefineCharacter(fqn, path);
    }
  );

  engine_namespace.set_function("RequiresCharacter",
    [this](const std::string& fqn) {
      // Handle built-ins...
      auto builtins = { "BuiltIns.Canodumb", "BuiltIns.Mettaur" };
      for (auto&& match : builtins) {
        if (fqn == match) return;
      }

      if (this->FetchCharacter(fqn) == nullptr) {
        std::string msg = "Failed to Require character with FQN " + fqn;
        Logger::Log(msg);
        throw std::runtime_error(msg);
      }
    }
  );

  const auto& tile_state_table = state.new_enum("TileState",
    "Broken", TileState::broken,
    "Cracked", TileState::cracked,
    "DirectionDown", TileState::directionDown,
    "DirectionLevel", TileState::directionLeft,
    "DirectionRight", TileState::directionRight,
    "DirectionUp", TileState::directionUp,
    "Empty", TileState::empty,
    "Grass", TileState::grass,
    "Hidden", TileState::hidden,
    "Holy", TileState::holy,
    "Ice", TileState::ice,
    "Lava", TileState::lava,
    "Normal", TileState::normal,
    "Poison", TileState::poison,
    "Volcano", TileState::volcano
  );

  const auto& defense_order_table = state.new_enum("DefenseOrder",
    "Always", DefenseOrder::always,
    "CollisionOnly", DefenseOrder::collisionOnly
  );

  const auto& particle_impact_type_table = state.new_enum("ParticleType",
    "Blue", ParticleImpact::Type::blue,
    "Fire", ParticleImpact::Type::fire,
    "Green", ParticleImpact::Type::green,
    "Thin", ParticleImpact::Type::thin,
    "Volcano", ParticleImpact::Type::volcano,
    "Vulcan", ParticleImpact::Type::vulcan,
    "Wind", ParticleImpact::Type::wind,
    "Yellow", ParticleImpact::Type::yellow
  );

  const auto& elements_table = state.new_enum("Element",
    "Fire", Element::fire,
    "Aqua", Element::aqua,
    "Elec", Element::elec,
    "Wood", Element::wood,
    "Sword", Element::sword,
    "Wind", Element::wind,
    "Cursor", Element::cursor,
    "Summon", Element::summon,
    "Plus", Element::plus,
    "Break", Element::breaker,
    "None", Element::none,
    "Ice", Element::ice
  );

  const auto& direction_table = state.new_enum("Direction",
    "None", Direction::none,
    "Up", Direction::up,
    "Down", Direction::down,
    "Left", Direction::left,
    "Right", Direction::right,
    "UpLeft", Direction::up_left,
    "UpRight", Direction::up_right,
    "DownLeft", Direction::down_left,
    "DownRight", Direction::down_right
  );

  const auto& animation_mode_record = state.new_enum("Playback",
    "Once", Animator::Mode::NoEffect,
    "Loop", Animator::Mode::Loop,
    "Bounce", Animator::Mode::Bounce,
    "Reverse", Animator::Mode::Reverse
  );

  const auto& team_record = state.new_enum("Team",
    "Red", Team::red,
    "Blue", Team::blue,
    "Other", Team::unknown
  );

  const auto& highlight_record = state.new_enum("Highlight",
    "Solid", Battle::Tile::Highlight::solid,
    "Flash", Battle::Tile::Highlight::flash,
    "None", Battle::Tile::Highlight::none
  );

  const auto& add_status_record = state.new_enum("EntityStatus",
    "Queued", Field::AddEntityStatus::queued,
    "Added", Field::AddEntityStatus::added,
    "Failed", Field::AddEntityStatus::deleted
  );

  const auto& action_order_record = state.new_enum("ActionOrder",
    "Involuntary", ActionOrder::involuntary,
    "Voluntary", ActionOrder::voluntary,
    "Immediate", ActionOrder::immediate
  );

  const auto& hitbox_flags_record = state.new_enum("Hit",
    "None", Hit::none,
    "Flinch", Hit::flinch,
    "Flash", Hit::flash,
    "Stun", Hit::stun,
    "Impact", Hit::impact,
    "Shake", Hit::shake,
    "Pierce", Hit::pierce,
    "Retangible", Hit::retangible,
    "Breaking", Hit::breaking,
    "Bubble", Hit::bubble,
    "Freeze", Hit::freeze,
    "Drag", Hit::drag
  );

  const auto& hitbox_drag_prop_record = state.new_usertype<Hit::Drag>("Drag",
    "direction", &Hit::Drag::dir,
    "count", &Hit::Drag::count
  );

  const auto& hitbox_props_record = state.new_usertype<Hit::Properties>("HitProps",
    "aggressor", &Hit::Properties::aggressor,
    "damage", &Hit::Properties::damage,
    "drag", &Hit::Properties::drag,
    "element", &Hit::Properties::element,
    "flags", &Hit::Properties::flags
  );

  state.set_function("MakeHitProps", 
    [](int damage, Hit::Flags flags, Element element, Entity::ID_t aggressor, Hit::Drag drag) {
      return Hit::Properties{
        damage, flags, element, aggressor, drag
      };
    }
  );

  state.set_function("Drag",
    [](Direction dir, unsigned count) {
      return Hit::Drag{ dir, count };
    }
  );

  state.set_function("frames",
    [](unsigned num) {
      return frames(num);
    }
  );
}

ScriptResourceManager& ScriptResourceManager::GetInstance()
{
    static ScriptResourceManager instance;
    return instance;
}

ScriptResourceManager::~ScriptResourceManager()
{
  scriptTableHash.clear();
  for (auto ptr : states) {
    delete ptr;
  }
  states.clear();
}

ScriptResourceManager::LoadScriptResult& ScriptResourceManager::LoadScript(const std::string& path)
{
  auto iter = scriptTableHash.find(path);

  if (iter != scriptTableHash.end()) {
    return iter->second;
  }

  sol::state* lua = new sol::state;
  ConfigureEnvironment(*lua);
  states.push_back(lua);

  auto load_result = lua->safe_script_file(path, sol::script_throw_on_error);
  auto pair = scriptTableHash.emplace(path, LoadScriptResult{std::move(load_result), lua} );
  return pair.first->second;
}

void ScriptResourceManager::DefineCharacter(const std::string& fqn, const std::string& path)
{
  auto iter = characterFQN.find(fqn);

  if (iter == characterFQN.end()) {
    auto& res = LoadScript(path+"/entry.lua");

    if (res.result.valid()) {
      characterFQN[fqn] = path;
    }
    else {
      sol::error error = res.result;
      Logger::Logf("Failed to Require character with FQN %s. Reason: %s", fqn.c_str(), error.what());

      // bubble up to end this scene from loading that needs this character...
      throw std::exception(error);
    }
  }
}

sol::state* ScriptResourceManager::FetchCharacter(const std::string& fqn)
{
  auto iter = characterFQN.find(fqn);

  if (iter != characterFQN.end()) {
    return LoadScript(iter->second+"/entry.lua").state;
  }

  // else miss
  return nullptr;
}

const std::string& ScriptResourceManager::CharacterToModpath(const std::string& fqn) {
  return characterFQN[fqn];
}
#endif