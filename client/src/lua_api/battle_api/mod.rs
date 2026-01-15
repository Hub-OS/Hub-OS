mod action_api;
mod animation_api;
mod augment_api;
mod battle_lua_api;
mod built_in_api;
mod card_damage_resolver;
mod card_select_api;
mod card_select_button_api;
mod component_api;
mod defense_rule_api;
mod desync_patch_api;
mod encounter_init;
mod entity_api;
mod errors;
mod field_api;
mod global_api;
mod movement_api;
mod player_form_api;
mod require_api;
mod resources_api;
mod sprite_api;
mod status_api;
mod sync_node_api;
mod tile_api;
mod tile_state_api;
mod turn_gauge_api;

pub use action_api::create_action_table;
pub use augment_api::create_augment_table;
pub use battle_lua_api::*;
pub use built_in_api::inject_internal_scripts;
pub use card_damage_resolver::*;
pub use card_select_button_api::*;
pub use encounter_init::*;
pub use entity_api::create_entity_table;
pub use movement_api::create_movement_table;
pub use status_api::create_status_table;
pub use tile_state_api::create_custom_tile_state_table;

// tables, most are stored as named registry values
// naming conflicts with simple registry values are avoided by using PascalCase
pub const GLOBAL_TABLE: &str = "_G";
pub const RESOURCES_TABLE: &str = "Resources";
pub const TURN_GAUGE_TABLE: &str = "TurnGauge";
pub const SPRITE_TABLE: &str = "Sprite";
pub const HUD_TABLE: &str = "Hud";
pub const TEXT_STYLE_TABLE: &str = "TextStyle";
pub const SYNC_NODE_TABLE: &str = "SyncNode";
pub const ANIMATION_TABLE: &str = "Animation";
pub const BATTLE_TABLE: &str = "Battle";
pub const ENCOUNTER_TABLE: &str = "Encounter";
pub const SPAWNER_TABLE: &str = "MobSpawner";
pub const MUTATOR_TABLE: &str = "Mutator";
pub const HIT_FLAG_TABLE: &str = "Hit";
pub const HIT_HELPER_TABLE: &str = "HitHelper";
pub const STATUS_TABLE: &str = "Status";
pub const DEFENSE_RULE_TABLE: &str = "DefenseRule";
pub const DEFENSE_TABLE: &str = "Defense";
pub const INTANGIBLE_RULE_TABLE: &str = "IntangibleRule";
pub const CARD_PROPERTIES_TABLE: &str = "CardProperties";
pub const ACTION_TABLE: &str = "Action";
pub const ACTION_STEP_TABLE: &str = "ActionStep";
pub const MOVEMENT_TABLE: &str = "Movement";
pub const ATTACHMENT_TABLE: &str = "Attachment";
pub const COMPONENT_TABLE: &str = "Component";
pub const FIELD_TABLE: &str = "Field";
pub const FIELD_COMPAT_TABLE: &str = "FieldCompat";
pub const TILE_TABLE: &str = "Tile";
pub const TILE_STATE_TABLE: &str = "TileState";
pub const CUSTOM_TILE_STATE_TABLE: &str = "CustomTileState";
pub const ENTITY_TABLE: &str = "Entity";
pub const LIVING_TABLE: &str = "Living";
pub const PLAYER_TABLE: &str = "Player";
pub const PLAYER_FORM_TABLE: &str = "PlayerForm";
pub const CHARACTER_TABLE: &str = "Character";
pub const OBSTACLE_TABLE: &str = "Obstacle";
pub const SPELL_TABLE: &str = "Spell";
pub const ARTIFACT_TABLE: &str = "Artifact";
pub const EXPLOSION_TABLE: &str = "Explosion";
pub const POOF_TABLE: &str = "Poof";
pub const ALERT_TABLE: &str = "Alert";
pub const TRAP_ALERT_TABLE: &str = "TrapAlert";
pub const BUSTER_TABLE: &str = "Buster";
pub const STANDARD_ENEMY_AUX_TABLE: &str = "StandardEnemyAux";
pub const HIT_PROPS_TABLE: &str = "HitProps";
pub const HITBOX_TABLE: &str = "Hitbox";
pub const SHARED_HITBOX_TABLE: &str = "SharedHitbox";
pub const AUGMENT_TABLE: &str = "Augment";
pub const CARD_SELECT_BUTTON_TABLE: &str = "CardSelectButton";
pub const AUX_PROP_TABLE: &str = "AuxProp";

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
const SPAWN_FN: &str = "on_spawn_func";
const IDLE_FN: &str = "on_idle_func";
const DELETE_FN: &str = "on_delete_func";
const BATTLE_START_FN: &str = "on_battle_start_func";
const BATTLE_END_FN: &str = "on_battle_end_func";
const COUNTER_FN: &str = "on_counter_func";

// living
const COUNTERED_FN: &str = "on_countered_func";

// characters
const INTRO_FN: &str = "intro_func";

// spells
const COLLISION_FN: &str = "on_collision_func";
const ATTACK_FN: &str = "on_attack_func";

// players
const MOVEMENT_INPUT_FN: &str = "movement_input_func";
const MOVEMENT_FN: &str = "movement_func";
const CHARGE_TIMING_FN: &str = "calculate_charge_time_func";
const NORMAL_ATTACK_FN: &str = "normal_attack_func";
const CHARGED_ATTACK_FN: &str = "charged_attack_func";
const SPECIAL_ATTACK_FN: &str = "special_attack_func";
const CHARGED_CARD_FN: &str = "charged_card_func";
const CARD_CHARGE_TIMING_FN: &str = "calculate_card_charge_time_func";
const BUILD_SPECIAL_CARD_FN: &str = "build_special_card_func";

// player forms
const ACTIVATE_FN: &str = "on_activate_func";
const DEACTIVATE_FN: &str = "on_deactivate_func";
const SELECT_FN: &str = "on_select_func";
const DESELECT_FN: &str = "on_deselect_func";

// buttons
const USE_FN: &str = "use_func";
const SELECTION_CHANGE_FN: &str = "on_selection_change_func";

// defense rules
pub const DEFENSE_FN: &str = "defense_func";
pub const FILTER_HIT_PROPS_FN: &str = "filter_func";

// defense rules + tile states
pub const REPLACE_FN: &str = "on_replace_func";

// tile states
pub const CAN_REPLACE_FN: &str = "can_replace_func";
pub const ENTITY_ENTER_FN: &str = "on_entity_enter_func";
pub const ENTITY_LEAVE_FN: &str = "on_entity_leave_func";
pub const ENTITY_STOP_FN: &str = "on_entity_stop_func";

// components
const INIT_FN: &str = "on_init_func";

// movement
pub const BEGIN_FN: &str = "on_begin_func";
pub const END_FN: &str = "on_end_func";

// globals, using named registry keys to avoid lua globals / implementation privacy
// naming conflicts with table names are avoided by using snake_case
pub const VM_INDEX_REGISTRY_KEY: &str = "vm_index";
pub const PACKAGE_ID_REGISTRY_KEY: &str = "pkg_id";
const TILE_CACHE_REGISTRY_KEY: &str = "tiles";
pub const GAME_FOLDER_REGISTRY_KEY: &str = "game_folder";
pub const LOADED_REGISTRY_KEY: &str = "loaded";
pub const MODULES_REGISTRY_KEY: &str = "modules";
