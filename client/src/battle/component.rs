use super::{BattleCallback, BattleSimulation, Entity};
use crate::battle::Artifact;
use crate::bindable::{ComponentLifetime, EntityId};
use crate::render::{FrameTime, SpriteShaderEffect};
use crate::structures::GenerationalIndex;
use framework::prelude::{Color, Vec2};
use rand::Rng;

#[derive(Clone)]
pub struct Component {
    pub entity: EntityId,
    pub lifetime: ComponentLifetime,
    pub update_callback: BattleCallback,
    pub init_callback: BattleCallback,
}

impl Component {
    pub fn new(entity: EntityId, lifetime: ComponentLifetime) -> Self {
        Self {
            entity,
            lifetime,
            update_callback: BattleCallback::default(),
            init_callback: BattleCallback::default(),
        }
    }

    /// Deletes the entity after a specified amount of battle frames
    pub fn create_delayed_deleter(
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
        lifetime: ComponentLifetime,
        delay: FrameTime,
    ) {
        let start_time = simulation.battle_time;

        simulation.components.insert_with_key(|index| {
            let mut component = Self::new(entity_id, lifetime);

            component.update_callback =
                BattleCallback::new(move |game_io, resources, simulation, _| {
                    if simulation.battle_time - start_time < delay {
                        return;
                    }

                    Entity::delete(game_io, resources, simulation, entity_id);
                    Component::eject(simulation, index);
                });

            component
        });
    }

    /// Causes the entity to rise, used for poof artifact
    pub fn create_float(simulation: &mut BattleSimulation, entity_id: EntityId) {
        let mut component = Self::new(entity_id, ComponentLifetime::Battle);

        component.update_callback = BattleCallback::new(move |_, _, simulation, _| {
            let entity = simulation
                .entities
                .query_one_mut::<&mut Entity>(entity_id.into())
                .unwrap();

            entity.offset.y -= 1.0;
        });

        simulation.components.insert(component);
    }

    pub fn create_player_deletion(simulation: &mut BattleSimulation, entity_id: EntityId) {
        const START_DELAY: FrameTime = 25;
        const TOTAL_DURATION: FrameTime = 50;

        let start_time = simulation.battle_time;

        let mut component = Self::new(entity_id, ComponentLifetime::Battle);

        component.update_callback =
            BattleCallback::new(move |game_io, resources, simulation, _| {
                let entity = (simulation.entities)
                    .query_one_mut::<&mut Entity>(entity_id.into())
                    .unwrap();

                let elapsed_time = simulation.battle_time - start_time;

                let alpha = if elapsed_time < START_DELAY {
                    1.0
                } else {
                    let progress =
                        (elapsed_time - START_DELAY) as f32 / (TOTAL_DURATION - START_DELAY) as f32;
                    1.0 - progress
                };

                if let Some(sprite_tree) = simulation.sprite_trees.get_mut(entity.sprite_tree_index)
                {
                    let root_node = sprite_tree.root_mut();
                    root_node.set_color(Color::WHITE.multiply_alpha(alpha));
                    root_node.set_shader_effect(SpriteShaderEffect::Pixelate);
                }

                if elapsed_time >= TOTAL_DURATION {
                    Entity::mark_erased(game_io, resources, simulation, entity_id);
                }
            });

        simulation.components.insert(component);
    }

    pub fn create_character_deletion(
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
        explosion_count: usize,
    ) {
        const END_DELAY: FrameTime = 4;
        const EXPLOSION_RATE: FrameTime = 14;

        let start_time = simulation.battle_time;
        let total_duration = EXPLOSION_RATE * explosion_count as FrameTime + END_DELAY;

        let mut component = Self::new(entity_id, ComponentLifetime::Battle);

        component.update_callback =
            BattleCallback::new(move |game_io, resources, simulation, _| {
                let elapsed_time = simulation.battle_time - start_time;

                // flash the entity white
                let entity = (simulation.entities)
                    .query_one_mut::<&mut Entity>(entity_id.into())
                    .unwrap();

                let color = if (elapsed_time / 2) % 2 == 0 {
                    Color::WHITE
                } else {
                    Color::BLACK
                };

                if let Some(sprite_tree) = simulation.sprite_trees.get_mut(entity.sprite_tree_index)
                {
                    let root_node = sprite_tree.root_mut();
                    root_node.set_color(color);
                }

                // spawn explosions
                let entity_x = entity.x;
                let entity_y = entity.y;
                let total_entity_offset =
                    entity.offset + entity.movement_offset + Vec2::new(0.0, entity.elevation);

                if elapsed_time % EXPLOSION_RATE == 0 && elapsed_time < total_duration - END_DELAY {
                    let id = Artifact::create_explosion(game_io, simulation);
                    let explosion_entity = simulation
                        .entities
                        .query_one_mut::<&mut Entity>(id.into())
                        .unwrap();

                    explosion_entity.pending_spawn = true;
                    explosion_entity.x = entity_x;
                    explosion_entity.y = entity_y;

                    let sprite_tree =
                        &mut simulation.sprite_trees[explosion_entity.sprite_tree_index];
                    sprite_tree.root_mut().set_layer(-1);

                    // random offset
                    let tile_size = simulation.field.tile_size();

                    explosion_entity.offset = Vec2::new(
                        simulation.rng.gen_range(-0.4..0.4),
                        simulation.rng.gen_range(-0.5..0.0),
                    );
                    explosion_entity.offset *= tile_size;
                    explosion_entity.offset += total_entity_offset;
                }

                if elapsed_time >= total_duration {
                    Entity::mark_erased(game_io, resources, simulation, entity_id);
                }
            });

        simulation.components.insert(component);
    }

    pub fn eject(simulation: &mut BattleSimulation, index: GenerationalIndex) {
        let Some(component) = simulation.components.remove(index) else {
            return;
        };

        if component.lifetime != ComponentLifetime::Local {
            return;
        }

        let entities = &mut simulation.entities;

        let Ok(entity) = entities.query_one_mut::<&mut Entity>(component.entity.into()) else {
            return;
        };

        let index = (entity.local_components)
            .iter()
            .position(|stored_index| *stored_index == index)
            .unwrap();

        entity.local_components.swap_remove(index);
    }
}
