use super::{BattleCallback, BattleSimulation, Entity};
use crate::bindable::{ComponentLifetime, EntityId};
use crate::render::FrameTime;
use crate::structures::GenerationalIndex;

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
