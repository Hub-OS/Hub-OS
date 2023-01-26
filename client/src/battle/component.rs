use super::{BattleCallback, BattleSimulation, Entity};
use crate::bindable::{ComponentLifetime, EntityId};
use crate::render::FrameTime;
use framework::prelude::{Color, Vec2};
use rand::Rng;

#[derive(Clone)]
pub struct Component {
    pub entity: EntityId,
    pub lifetime: ComponentLifetime,
    pub update_callback: BattleCallback,
}

impl Component {
    pub fn new(entity: EntityId, lifetime: ComponentLifetime) -> Self {
        Self {
            entity,
            lifetime,
            update_callback: BattleCallback::default(),
        }
    }

    pub fn new_player_deletion(simulation: &mut BattleSimulation, entity_id: EntityId) {
        const START_DELAY: FrameTime = 25;
        const TOTAL_DURATION: FrameTime = 50;

        let start_time = simulation.battle_time;

        simulation.components.insert_with(move |index| {
            let mut component = Self::new(entity_id, ComponentLifetime::BattleStep);

            component.update_callback = BattleCallback::new(move |_, simulation, _, _| {
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

                let root_node = entity.sprite_tree.root_mut();
                root_node.set_color(Color::WHITE.multiply_alpha(alpha));

                if elapsed_time >= TOTAL_DURATION {
                    entity.erased = true;
                    simulation.components.remove(index);
                }
            });

            component
        });
    }

    pub fn new_character_deletion(
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
        explosion_count: usize,
    ) {
        const END_DELAY: FrameTime = 4;
        const EXPLOSION_RATE: FrameTime = 14;

        let start_time = simulation.battle_time;
        let total_duration = EXPLOSION_RATE * explosion_count as FrameTime + END_DELAY;

        simulation.components.insert_with(move |index| {
            let mut component = Self::new(entity_id, ComponentLifetime::BattleStep);

            component.update_callback = BattleCallback::new(move |game_io, simulation, _, _| {
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

                let root_node = entity.sprite_tree.root_mut();
                root_node.set_color(color);

                if elapsed_time >= total_duration {
                    entity.erased = true;
                    simulation.components.remove(index);
                }

                // spawn explosions
                let entity_x = entity.x;
                let entity_y = entity.y;
                let total_entity_offset =
                    entity.offset + entity.tile_offset + Vec2::new(0.0, entity.elevation);

                if elapsed_time % EXPLOSION_RATE == 0 && elapsed_time < total_duration - END_DELAY {
                    let id = simulation.create_explosion(game_io);
                    let explosion_entity = simulation
                        .entities
                        .query_one_mut::<&mut Entity>(id.into())
                        .unwrap();

                    explosion_entity.pending_spawn = true;
                    explosion_entity.x = entity_x;
                    explosion_entity.y = entity_y;
                    explosion_entity.sprite_tree.root_mut().set_layer(-1);

                    // random offset
                    let tile_size = simulation.field.tile_size();

                    explosion_entity.offset = Vec2::new(
                        simulation.rng.gen_range(-0.4..0.4),
                        simulation.rng.gen_range(-0.5..0.0),
                    );
                    explosion_entity.offset *= tile_size;
                    explosion_entity.offset += total_entity_offset;
                }
            });

            component
        });
    }
}
