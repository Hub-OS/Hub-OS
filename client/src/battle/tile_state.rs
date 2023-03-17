use super::{BattleCallback, Entity, Living};
use crate::bindable::{Element, EntityId, HitFlag, HitProperties, MoveAction};
use crate::render::FrameTime;
use crate::resources::{
    Globals, BROKEN_LIFETIME, CONVEYOR_LIFETIME, CONVEYOR_SLIDE_DURATION, CONVEYOR_WAIT_DELAY,
    GRASS_HEAL_INTERVAL, GRASS_SLOWED_HEAL_INTERVAL, POISON_INTERVAL,
};
use packets::structures::Direction;

#[derive(Clone)]
pub struct TileState {
    pub default_animation_state: String,
    pub flipped_animation_state: String,
    pub cleanser_element: Option<Element>,
    pub is_hole: bool,
    pub max_lifetime: Option<FrameTime>,
    pub change_request_callback: BattleCallback<usize, bool>,
    pub entity_enter_callback: BattleCallback<EntityId>,
    pub entity_leave_callback: BattleCallback<(EntityId, MoveAction)>,
    pub entity_stop_callback: BattleCallback<(EntityId, MoveAction)>,
    pub entity_update_callback: BattleCallback<EntityId>,
    // pub card_use_callback: BattleCallback,
    pub calculate_bonus_damage_callback: BattleCallback<(HitProperties, i32), i32>,
}

impl TileState {
    pub const NORMAL: usize = 0;
    pub const CRACKED: usize = 1;
    pub const BROKEN: usize = 2;
    pub const ICE: usize = 3;
    pub const GRASS: usize = 4;
    pub const LAVA: usize = 5;
    pub const POISON: usize = 6;
    pub const EMPTY: usize = 7;
    pub const HOLY: usize = 8;
    pub const DIRECTION_LEFT: usize = 9;
    pub const DIRECTION_RIGHT: usize = 10;
    pub const DIRECTION_UP: usize = 11;
    pub const DIRECTION_DOWN: usize = 12;
    pub const VOLCANO: usize = 13;
    pub const SEA: usize = 14;
    pub const SAND: usize = 15;
    pub const METAL: usize = 16;
    pub const HIDDEN: usize = 17;
    const TOTAL_BUILT_IN: usize = 18;

    pub fn new(state: String) -> Self {
        Self::new_directional(state.clone(), state)
    }

    pub fn new_directional(default_state: String, flipped_state: String) -> Self {
        Self {
            default_animation_state: default_state,
            flipped_animation_state: flipped_state,
            cleanser_element: None,
            is_hole: false,
            max_lifetime: None,
            change_request_callback: BattleCallback::stub(true),
            entity_enter_callback: BattleCallback::default(),
            entity_leave_callback: BattleCallback::default(),
            entity_stop_callback: BattleCallback::default(),
            entity_update_callback: BattleCallback::default(),
            calculate_bonus_damage_callback: BattleCallback::default(),
        }
    }

    pub fn create_registry() -> Vec<TileState> {
        let mut registry = Vec::new();

        // Normal
        let normal_state = TileState::new(String::from("normal"));
        registry.push(normal_state);

        // Cracked
        let mut cracked_state = TileState::new(String::from("cracked"));

        cracked_state.entity_leave_callback = BattleCallback::new(
            |game_io, simulation, _, (_, move_action): (EntityId, MoveAction)| {
                let Some(tile) = simulation.field.tile_at_mut(move_action.source) else {
                    return;
                };

                if tile.reservations().is_empty() {
                    if !simulation.is_resimulation {
                        let globals = game_io.resource::<Globals>().unwrap();
                        globals.audio.play_sound(&globals.tile_break_sfx);
                    }

                    tile.set_state_index(TileState::BROKEN, None);
                }
            },
        );

        registry.push(cracked_state);

        // Broken
        let mut broken_state = TileState::new(String::from("broken"));
        broken_state.is_hole = true;
        broken_state.max_lifetime = Some(BROKEN_LIFETIME);
        registry.push(broken_state);

        // Ice
        let mut ice_state = TileState::new(String::from("ice"));

        ice_state.entity_stop_callback = BattleCallback::new(
            |game_io, simulation, vms, (entity_id, move_action): (EntityId, MoveAction)| {
                let entities = &mut simulation.entities;
                let Ok(entity) = entities.query_one_mut::<&Entity>(entity_id.into()) else {
                    return;
                };

                if entity.element == Element::Aqua {
                    return;
                }

                let direction = move_action.aligned_direction();
                let offset = direction.i32_vector();
                let dest = (entity.x + offset.0, entity.y + offset.1);

                let can_move_to_callback =
                    entity.current_can_move_to_callback(&simulation.card_actions);

                if !can_move_to_callback.call(game_io, simulation, vms, dest) {
                    return;
                }

                let entities = &mut simulation.entities;

                if let Ok(entity) = entities.query_one_mut::<&mut Entity>(entity_id.into()) {
                    entity.move_action = Some(MoveAction::slide(dest, 4));
                }
            },
        );

        registry.push(ice_state);

        // Grass
        let mut grass_state = TileState::new(String::from("grass"));
        grass_state.cleanser_element = Some(Element::Fire);

        grass_state.calculate_bonus_damage_callback = BattleCallback::new(
            |_, _, _, (hit_props, original_damage): (HitProperties, i32)| {
                if hit_props.is_super_effective(Element::Wood) {
                    original_damage
                } else {
                    0
                }
            },
        );

        grass_state.entity_update_callback = BattleCallback::new(
            |_, simulation, _, entity_id: EntityId| {
                let entities = &mut simulation.entities;
                let Ok((entity, living)) = entities.query_one_mut::<(&Entity, &mut Living)>(entity_id.into()) else {
                    return;
                };

                if entity.element != Element::Wood || living.health >= living.max_health {
                    return;
                }
                // todo: should entities have individual timers?
                let heal_interval = if living.health >= 9 {
                    GRASS_HEAL_INTERVAL
                } else {
                    GRASS_SLOWED_HEAL_INTERVAL
                };

                if simulation.battle_time > 0 && simulation.battle_time % heal_interval == 0 {
                    living.health += 1;
                }
            },
        );

        registry.push(grass_state);

        // Lava
        let mut lava_state = TileState::new(String::from("lava"));

        lava_state.calculate_bonus_damage_callback = BattleCallback::new(
            |_, _, _, (hit_props, original_damage): (HitProperties, i32)| {
                if hit_props.is_super_effective(Element::Fire) {
                    original_damage
                } else {
                    0
                }
            },
        );

        lava_state.entity_update_callback =
            BattleCallback::new(|game_io, simulation, vms, entity_id: EntityId| {
                let entities = &mut simulation.entities;
                let Ok(entity) = entities.query_one_mut::<&Entity>(entity_id.into()) else {
                    return;
                };

                if entity.element == Element::Fire {
                    return;
                }

                let position = (entity.x, entity.y);

                // todo: spawn explosion artifact

                let hit_props = HitProperties {
                    damage: 50,
                    element: Element::Fire,
                    ..Default::default()
                };

                Living::process_hit(game_io, simulation, vms, entity_id, hit_props);

                if let Some(tile) = simulation.field.tile_at_mut(position) {
                    tile.set_state_index(TileState::NORMAL, None);
                }
            });

        registry.push(lava_state);

        // Poison
        let mut poison_state = TileState::new(String::from("poison"));

        poison_state.entity_enter_callback =
            BattleCallback::new(|game_io, simulation, vms, entity_id: EntityId| {
                // take 1hp immediately for stepping on poison
                let mut hit_props = HitProperties::blank();
                hit_props.damage = 1;

                Living::process_hit(game_io, simulation, vms, entity_id, hit_props);
            });

        poison_state.entity_update_callback =
            BattleCallback::new(|game_io, simulation, vms, entity_id: EntityId| {
                if simulation.battle_time > 0 && simulation.battle_time % POISON_INTERVAL == 0 {
                    let mut hit_props = HitProperties::blank();
                    hit_props.damage = 1;

                    Living::process_hit(game_io, simulation, vms, entity_id, hit_props);
                }
            });

        registry.push(poison_state);

        // Empty
        let mut empty_state = TileState::new(String::from("empty"));
        empty_state.is_hole = true;
        registry.push(empty_state);

        // Holy
        let mut holy_state = TileState::new(String::from("holy"));

        holy_state.calculate_bonus_damage_callback =
            BattleCallback::new(|_, _, _, (hit_props, _): (HitProperties, i32)| {
                -hit_props.damage / 2
            });

        registry.push(holy_state);

        // conveyors
        let generate_conveyor_update = |direction: Direction| {
            let offset = direction.i32_vector();

            BattleCallback::new(move |game_io, simulation, vms, entity_id: EntityId| {
                let entities = &mut simulation.entities;
                let Ok(entity) = entities.query_one_mut::<&mut Entity>(entity_id.into()) else{
                    return;
                };

                let elapsed_since_movement = simulation.battle_time - entity.last_movement;

                if entity.move_action.is_some() || elapsed_since_movement < CONVEYOR_WAIT_DELAY {
                    return;
                }

                let dest = (entity.x + offset.0, entity.y + offset.1);

                let can_move_to_callback =
                    entity.current_can_move_to_callback(&simulation.card_actions);

                if !can_move_to_callback.call(game_io, simulation, vms, dest) {
                    return;
                }

                let entities = &mut simulation.entities;
                if let Ok(entity) = entities.query_one_mut::<&mut Entity>(entity_id.into()) {
                    let move_action = MoveAction::slide(dest, CONVEYOR_SLIDE_DURATION);
                    entity.move_action = Some(move_action);
                }
            })
        };

        // DirectionLeft
        let mut direction_left_state = TileState::new_directional(
            String::from("direction_left"),
            String::from("direction_right"),
        );
        direction_left_state.cleanser_element = Some(Element::Wood);
        direction_left_state.max_lifetime = Some(CONVEYOR_LIFETIME);
        direction_left_state.entity_update_callback = generate_conveyor_update(Direction::Left);

        registry.push(direction_left_state);

        // DirectionRight
        let mut direction_right_state = TileState::new_directional(
            String::from("direction_right"),
            String::from("direction_left"),
        );
        direction_right_state.cleanser_element = Some(Element::Wood);
        direction_right_state.max_lifetime = Some(CONVEYOR_LIFETIME);
        direction_right_state.entity_update_callback = generate_conveyor_update(Direction::Right);

        registry.push(direction_right_state);

        // DirectionUp
        let mut direction_up_state = TileState::new(String::from("direction_up"));
        direction_up_state.cleanser_element = Some(Element::Wood);
        direction_up_state.max_lifetime = Some(CONVEYOR_LIFETIME);
        direction_up_state.entity_update_callback = generate_conveyor_update(Direction::Up);
        registry.push(direction_up_state);

        // DirectionDown
        let mut direction_down_state = TileState::new(String::from("direction_down"));
        direction_down_state.cleanser_element = Some(Element::Wood);
        direction_down_state.max_lifetime = Some(CONVEYOR_LIFETIME);
        direction_down_state.entity_update_callback = generate_conveyor_update(Direction::Down);
        registry.push(direction_down_state);

        // Volcano
        let mut volcano_state = TileState::new(String::from("volcano"));
        volcano_state.cleanser_element = Some(Element::Aqua);
        registry.push(volcano_state);

        // Sea
        let mut sea_state = TileState::new(String::from("sea"));

        sea_state.entity_stop_callback = BattleCallback::new(
            |game_io, simulation, _vms, (entity_id, _): (EntityId, MoveAction)| {
                let entities = &mut simulation.entities;
                let Ok((entity, living)) = entities.query_one_mut::<(&Entity, &mut Living)>(entity_id.into()) else {
                    return;
                };

                if entity.element == Element::Aqua {
                    return;
                }

                let position = (entity.x, entity.y);

                living.status_director.apply_status(HitFlag::ROOT, 20);

                let splash_id = simulation.create_splash(game_io);
                simulation.request_entity_spawn(splash_id, position);
            },
        );

        sea_state.calculate_bonus_damage_callback = BattleCallback::new(
            |_, _, _, (hit_props, original_damage): (HitProperties, i32)| {
                if hit_props.is_super_effective(Element::Aqua) {
                    original_damage
                } else {
                    0
                }
            },
        );

        registry.push(sea_state);

        // Sand
        let mut sand_state = TileState::new(String::from("sand"));
        sand_state.cleanser_element = Some(Element::Wind);
        registry.push(sand_state);

        // Metal
        let mut metal_state = TileState::new(String::from("metal"));

        metal_state.change_request_callback =
            BattleCallback::new(|_, _, _, state_index: usize| state_index != TileState::CRACKED);

        registry.push(metal_state);

        // Hidden
        let mut hidden_state = TileState::new(String::from(""));
        hidden_state.is_hole = true;
        hidden_state.change_request_callback = BattleCallback::stub(false);
        registry.push(hidden_state);

        registry
    }
}
