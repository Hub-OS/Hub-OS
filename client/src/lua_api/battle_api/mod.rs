mod action_api;
mod animation_api;
mod augment_api;
mod battle_lua_api;
mod built_in_api;
mod component_api;
mod defense_rule_api;
mod encounter_init;
mod entity_api;
mod errors;
mod field_api;
mod global_api;
mod math_api;
mod movement_api;
mod player_form_api;
mod require_api;
mod resources_api;
mod sprite_api;
mod sync_node_api;
mod tile_api;
mod turn_gauge_api;

pub use augment_api::create_augment_table;
pub use battle_lua_api::*;
pub use encounter_init::encounter_init;
pub use entity_api::create_entity_table;

// tables, most are stored as named registry values
// naming conflicts with simple registry values are avoided by using PascalCase
pub const GLOBAL_TABLE: &str = "_G";
pub const RESOURCES_TABLE: &str = "Resources";
pub const TURN_GAUGE_TABLE: &str = "TurnGauge";
pub const SPRITE_TABLE: &str = "SpriteNode";
pub const SYNC_NODE_TABLE: &str = "SyncNode";
pub const ANIMATION_TABLE: &str = "Animation";
pub const BATTLE_TABLE: &str = "Battle";
pub const ENCOUNTER_TABLE: &str = "Encounter";
pub const SPAWNER_TABLE: &str = "MobSpawner";
pub const MUTATOR_TABLE: &str = "Mutator";
pub const DEFENSE_RULE_TABLE: &str = "DefenseRule";
pub const DEFENSE_JUDGE_TABLE: &str = "DefenseJudge";
pub const INTANGIBLE_RULE_TABLE: &str = "IntangibleRule";
pub const CARD_PROPERTIES_TABLE: &str = "CardProperties";
pub const ACTION_TABLE: &str = "Action";
pub const STEP_TABLE: &str = "Step";
pub const MOVEMENT_TABLE: &str = "Movement";
pub const ATTACHMENT_TABLE: &str = "Attachment";
pub const COMPONENT_TABLE: &str = "Component";
pub const FIELD_TABLE: &str = "Field";
pub const TILE_TABLE: &str = "Tile";
pub const ENTITY_TABLE: &str = "Entity";
pub const PLAYER_TABLE: &str = "Player";
pub const PLAYER_FORM_TABLE: &str = "PlayerForm";
pub const CHARACTER_TABLE: &str = "Character";
pub const OBSTACLE_TABLE: &str = "Obstacle";
pub const SPELL_TABLE: &str = "Spell";
pub const ARTIFACT_TABLE: &str = "Artifact";
pub const EXPLOSION_TABLE: &str = "Explosion";
pub const POOF_TABLE: &str = "Poof";
pub const ALERT_TABLE: &str = "Alert";
pub const BUSTER_TABLE: &str = "Buster";
pub const VIRUS_DEFENSE_TABLE: &str = "DefenseVirusBody";
pub const HIT_PROPS_TABLE: &str = "HitProps";
pub const HITBOX_TABLE: &str = "Hitbox";
pub const SHARED_HITBOX_TABLE: &str = "SharedHitbox";
pub const AUGMENT_TABLE: &str = "Augment";

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
const COUNTER_FN: &str = "on_counter_func";

// living
const COUNTERED_FN: &str = "on_countered_func";

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

// globals, using named registry keys to avoid lua globals / implementation privacy
// naming conflicts with table names are avoided by using snake_case
pub const VM_INDEX_REGISTRY_KEY: &str = "vm_index";
pub const NAMESPACE_REGISTRY_KEY: &str = "namespace";
const DELEGATE_TYPE_REGISTRY_KEY: &str = "delegate";
const DELEGATE_REGISTRY_KEY: &str = "delegate_type";
const TILE_CACHE_REGISTRY_KEY: &str = "tiles";
