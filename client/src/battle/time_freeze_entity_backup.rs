use super::{ActionQueue, BattleAnimator, BattleSimulation, Entity, Living, Movement};
use crate::battle::{SharedBattleResources, StatusDirector};
use crate::bindable::{Drag, EntityId, HitFlag, HitFlags};
use crate::render::FrameTime;
use crate::structures::GenerationalIndex;

#[derive(Clone)]
pub struct TimeFreezeEntityBackup {
    pub entity_id: EntityId,
    pub action_queue: ActionQueue,
    pub movement: Option<Movement>,
    pub animator: BattleAnimator,
    pub statuses: Vec<(HitFlags, FrameTime, bool)>,
    pub drag: Option<Drag>,
    pub drag_lockout: u8,
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

        if entity.deleted {
            return None;
        }

        entity.time_frozen = false;

        // swap action index
        let old_action_queue = std::mem::take(action_queue);

        action_queue.action_type = old_action_queue.action_type;
        action_queue.active = if simulation.actions.contains_key(action_index) {
            Some(action_index)
        } else {
            None
        };

        // back up animator
        let animator = &mut simulation.animators[entity.animator_index];
        let animator_backup = animator.clone();

        // reset callbacks as they're already stored in the original animator
        animator.clear_callbacks();

        // back up status_director
        let (statuses, drag, drag_lockout) = living
            .map(|living| {
                let take_flags = !HitFlag::KEEP_IN_FREEZE;
                let status_director = &mut living.status_director;

                let status_sprites = status_director.take_status_sprites(take_flags);

                if let Some(sprite_tree) = simulation.sprite_trees.get_mut(entity.sprite_tree_index)
                {
                    // delete old status sprites
                    for (_, index) in status_sprites {
                        sprite_tree.remove(index);
                    }
                }

                // track old statuses
                let mut statuses: Vec<_> = status_director
                    .applied()
                    .filter(|(flag, _)| *flag & take_flags != 0)
                    .map(|(flag, duration)| (flag, duration, true))
                    .collect();

                statuses.extend(
                    status_director
                        .pending()
                        .filter(|(flag, _)| *flag & take_flags != 0)
                        .map(|(flag, duration)| (flag, duration, false)),
                );

                let drag = status_director.take_drag_for_backup();
                let drag_lockout = status_director.remaining_drag_lockout();

                // clear existing statuses and call destructors to get rid of scripted effects and artifacts
                status_director.clear_statuses(take_flags);
                let destructors = status_director.take_ready_destructors();
                simulation.pending_callbacks.extend(destructors);

                (statuses, drag, drag_lockout)
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
            drag,
            drag_lockout,
        })
    }

    pub fn restore(mut self, simulation: &mut BattleSimulation, resources: &SharedBattleResources) {
        // fully restore the entity
        let entities = &mut simulation.entities;
        let id = self.entity_id.into();

        let Ok((entity, living)) = entities.query_one_mut::<(&mut Entity, Option<&mut Living>)>(id)
        else {
            return;
        };

        // freeze the entity again
        entity.time_frozen = true;

        // restore animator
        simulation.animators[entity.animator_index] = self.animator;

        // restore statuses, happens before movement since this can modify movement
        if let Some(living) = living {
            // merge to retain statuses applied during time freeze
            let status_director = &mut living.status_director;

            for (flag, duration, reapply) in self.statuses {
                if reapply {
                    status_director.reapply_status(flag, duration);
                } else {
                    status_director.apply_status(flag, duration);
                }
            }

            let movement_is_drag = self.drag.is_some() || self.drag_lockout > 0;

            if let Some(drag) = self.drag {
                status_director.set_drag(drag);
            }

            status_director.set_remaining_drag_lockout(self.drag_lockout);

            // must be called after entity.time_frozen = true
            StatusDirector::apply_status_changes(
                &resources.status_registry,
                &mut simulation.pending_callbacks,
                entity,
                living,
                &mut self.movement.as_mut().filter(|_| !movement_is_drag),
                self.entity_id,
            );
        }

        // restore the movement
        if let Some(movement) = self.movement {
            let _ = entities.insert_one(id, movement);
        }

        // restore action queue
        let mut old_queue = self.action_queue;

        if old_queue.active.is_some() || !old_queue.pending.is_empty() {
            if let Ok(existing_queue) = entities.query_one_mut::<&mut ActionQueue>(id) {
                if existing_queue.active.is_some() {
                    log::error!("Active action overwritten by TimeFreezeEntityBackup");
                }

                // avoid restoring deleted active actions
                existing_queue.active = old_queue
                    .active
                    .filter(|&index| simulation.actions.contains_key(index));

                // avoid restoring deleted queued actions
                old_queue
                    .pending
                    .retain(|&index| simulation.actions.contains_key(index));

                if old_queue.active.is_some() || !old_queue.pending.is_empty() {
                    // adopt old action type
                    existing_queue.action_type = old_queue.action_type;
                }

                if existing_queue.pending.is_empty() {
                    // use old queue
                    existing_queue.pending = old_queue.pending;
                } else {
                    // extend with old queue
                    existing_queue.pending.extend(old_queue.pending);
                }
            } else {
                let _ = entities.insert_one(id, old_queue);
            }
        }
    }
}
