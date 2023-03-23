mod action_api;
mod animation_api;
mod augment_api;
mod battle_init;
mod battle_lua_api;
mod built_in_api;
mod component_api;
mod defense_rule_api;
mod engine_api;
mod entity_api;
mod errors;
mod field_api;
mod global_api;
mod math_api;
mod movement_api;
mod player_form_api;
mod require_api;
mod sprite_api;
mod sync_node_api;
mod tile_api;

pub use augment_api::create_augment_table;
pub use battle_init::battle_init;
pub use battle_lua_api::*;
pub use entity_api::create_entity_table;

// tables
pub const GLOBAL_TABLE: &str = "_G";
pub const ENGINE_TABLE: &str = "Engine";
pub const SPRITE_TABLE: &str = "SpriteNode";
pub const SYNC_NODE_TABLE: &str = "SyncNode";
pub const ANIMATION_TABLE: &str = "Engine.Animation";
pub const BATTLE_TABLE: &str = "Battle";
pub const BATTLE_INIT_TABLE: &str = "Battle.BattleInit";
pub const SPAWNER_TABLE: &str = "Battle.MobSpawner";
pub const MUTATOR_TABLE: &str = "Battle.Mutator";
pub const DEFENSE_RULE_TABLE: &str = "Battle.DefenseRule";
pub const DEFENSE_JUDGE_TABLE: &str = "Battle.DefenseJudge";
pub const INTANGIBLE_RULE_TABLE: &str = "Battle.DefenseRule";
pub const CARD_PROPERTIES_TABLE: &str = "Battle.CardProperties";
pub const ACTION_TABLE: &str = "Battle.Action";
pub const STEP_TABLE: &str = "Battle.Step";
pub const MOVEMENT_TABLE: &str = "Battle.Movement";
pub const ATTACHMENT_TABLE: &str = "Battle.Attachment";
pub const COMPONENT_TABLE: &str = "Battle.Component";
pub const FIELD_TABLE: &str = "Battle.Field";
pub const TILE_TABLE: &str = "Battle.Tile";
pub const ENTITY_TABLE: &str = "Battle.Entity";
pub const PLAYER_TABLE: &str = "Battle.Player";
pub const PLAYER_FORM_TABLE: &str = "Battle.PlayerForm";
pub const CHARACTER_TABLE: &str = "Battle.Character";
pub const OBSTACLE_TABLE: &str = "Battle.Obstacle";
pub const SPELL_TABLE: &str = "Battle.Spell";
pub const ARTIFACT_TABLE: &str = "Battle.Artifact";
pub const EXPLOSION_TABLE: &str = "Battle.Explosion";
pub const POOF_TABLE: &str = "Battle.Poof";
pub const ALERT_TABLE: &str = "Battle.Alert";
pub const BUSTER_TABLE: &str = "Battle.Buster";
pub const VIRUS_DEFENSE_TABLE: &str = "Battle.DefenseVirusBody";
pub const HIT_PROPS_TABLE: &str = "Battle.HitProps";
pub const HITBOX_TABLE: &str = "Battle.Hitbox";
pub const SHARED_HITBOX_TABLE: &str = "Battle.SharedHitbox";
pub const AUGMENT_TABLE: &str = "Battle.Augment";

// callbacks

// entities + components + card actions
pub const UPDATE_FN: &str = "on_update_func";

// entities + card actions
const CAN_MOVE_TO_FN: &str = "can_move_to_func";

// card actions
const ANIMATION_END_FN: &str = "on_animation_end_func";
const ACTION_END_FN: &str = "on_action_end_func";
const EXECUTE_FN: &str = "on_execute_func";

// all entities
const DELETE_FN: &str = "on_delete_func";
const BATTLE_START_FN: &str = "on_battle_start_func";
const BATTLE_END_FN: &str = "on_battle_end_func";

// spells
const COLLISION_FN: &str = "on_collision_func";
const ATTACK_FN: &str = "on_attack_func";
const SPAWN_FN: &str = "on_spawn_func";

// players
const CHARGE_TIMING_FN: &str = "calculate_charge_time_func";
const NORMAL_ATTACK_FN: &str = "normal_attack_func";
const CHARGED_ATTACK_FN: &str = "charged_attack_func";
const SPECIAL_ATTACK_FN: &str = "special_attack_func";
const BUILD_SPECIAL_CARD_FN: &str = "build_special_card_func";

// components
const INIT_FN: &str = "on_init_func";
