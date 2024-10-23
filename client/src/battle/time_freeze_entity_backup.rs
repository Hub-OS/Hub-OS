use super::{
    Action, ActionQueue, BattleAnimator, BattleSimulation, Entity, Living, Movement,
    SharedBattleResources,
};
use crate::bindable::{EntityId, HitFlags};
use crate::render::FrameTime;
use crate::structures::GenerationalIndex;
use framework::prelude::GameIO;

#[derive(Clone)]
pub struct TimeFreezeEntityBackup {
    pub entity_id: EntityId,
    pub action_queue: ActionQueue,
    pub movement: Option<Movement>,
    pub animator: BattleAnimator,
    pub statuses: Vec<(HitFlags, FrameTime)>,
}

impl TimeFreezeEntityBackup {
    pub fn backup_and_prepare(
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
        action_index: GenerationalIndex,
    ) -> Option<Self> {
        let entities = &mut simulation.entities;

        ActionQueue::ensure(entities, entity_id);

        let Ok((entity, living, action_queue)) =
            entities.query_one_mut::<(&mut Entity, Option<&mut Living>, &mut ActionQueue)>(
                entity_id.into(),
            )
        else {
            return None;
        };

        entity.time_frozen = false;

        // swap action index
        let old_action_queue = std::mem::take(action_queue);
        action_queue.active = Some(action_index);

        // back up animator
        let animator = &mut simulation.animators[entity.animator_index];
        let animator_backup = animator.clone();

        // reset callbacks as they're already stored in the original animator
        animator.clear_callbacks();

        // back up status_director
        let statuses = living
            .map(|living| {
                let status_sprites = living.status_director.take_status_sprites();

                if let Some(sprite_tree) = simulation.sprite_trees.get_mut(entity.sprite_tree_index)
                {
                    // delete old status sprites
                    for (_, index) in status_sprites {
                        sprite_tree.remove(index);
                    }
                }

                let statuses = living.status_director.applied_and_pending();

                // clear existing statuses and call destructors to get rid of scripted effects and artifacts
                living.status_director.clear_statuses();
                let destructors = living.status_director.take_ready_destructors();
                simulation.pending_callbacks.extend(destructors);

                statuses
            })
            .unwrap_or_default();

        // back up movement
        let movement = entities.remove_one::<Movement>(entity_id.into()).ok();

        Some(Self {
            entity_id,
            action_queue: old_action_queue,
            movement,
            animator: animator_backup,
            statuses,
        })
    }

    pub fn restore(
        self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        // delete action if it still exists
        Action::cancel_all(game_io, resources, simulation, self.entity_id);

        // restore action queue
        let entities = &mut simulation.entities;
        let id = self.entity_id.into();

        if self.action_queue.active.is_some() || !self.action_queue.pending.is_empty() {
            let _ = entities.insert_one(id, self.action_queue);
        }

        // fully restore the entity
        let Ok((entity, living)) = entities.query_one_mut::<(&mut Entity, Option<&mut Living>)>(id)
        else {
            return;
        };

        // freeze the entity again
        entity.time_frozen = true;

        // restore animator
        simulation.animators[entity.animator_index] = self.animator;

        // restore statuses
        if let Some(living) = living {
            // merge to retain statuses applied during time freeze
            for (flag, duration) in self.statuses {
                living.status_director.apply_status(flag, duration);
            }
        }

        // restore the movement
        if let Some(movement) = self.movement {
            let _ = simulation.entities.insert_one(id, movement);
        }
    }
}
