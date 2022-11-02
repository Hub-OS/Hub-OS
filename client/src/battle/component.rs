use super::{BattleCallback, BattleSimulation, Entity};
use crate::bindable::{ComponentLifetime, EntityID};
use crate::render::FrameTime;
use framework::prelude::Color;

#[derive(Clone)]
pub struct Component {
    pub entity: EntityID,
    pub lifetime: ComponentLifetime,
    pub update_callback: BattleCallback,
}

impl Component {
    pub fn new(entity: EntityID, lifetime: ComponentLifetime) -> Self {
        Self {
            entity,
            lifetime,
            update_callback: BattleCallback::default(),
        }
    }

    pub fn new_character_deletion(simulation: &mut BattleSimulation, entity_id: EntityID) {
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
}
