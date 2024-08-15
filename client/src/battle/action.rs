use super::{
    BattleAnimator, BattleCallback, BattleScriptContext, BattleSimulation, Character, Entity,
    Field, Living, Movement, SharedBattleResources,
};
use crate::bindable::{
    ActionLockout, CardProperties, EntityId, GenerationalIndex, HitFlag, SpriteColorMode,
};
use crate::lua_api::create_entity_table;
use crate::packages::PackageNamespace;
use crate::render::{DerivedFrame, FrameTime, SpriteNode, Tree};
use crate::resources::Globals;
use crate::structures::SlotMap;
use framework::prelude::GameIO;
use std::cell::RefCell;

#[derive(Clone)]
pub struct Action {
    pub active_frames: FrameTime,
    pub processed: bool,
    pub executed: bool,
    pub deleted: bool,
    pub entity: EntityId,
    pub state: String,
    pub frame_callbacks: Vec<(usize, BattleCallback)>,
    pub sprite_index: GenerationalIndex,
    pub properties: CardProperties,
    pub derived_frames: Option<Vec<DerivedFrame>>,
    pub steps: Vec<ActionStep>,
    pub step_index: usize,
    pub attachments: Vec<ActionAttachment>,
    pub lockout_type: ActionLockout,
    pub old_position: (i32, i32),
    pub can_move_to_callback: Option<BattleCallback<(i32, i32), bool>>,
    pub update_callback: Option<BattleCallback>,
    pub execute_callback: Option<BattleCallback>,
    pub end_callback: Option<BattleCallback>,
    pub animation_end_callback: Option<BattleCallback>,
}

impl Action {
    fn new(entity_id: EntityId, state: String, sprite_index: GenerationalIndex) -> Self {
        Self {
            active_frames: 0,
            processed: false,
            executed: false,
            deleted: false,
            entity: entity_id,
            state,
            frame_callbacks: Vec::new(),
            sprite_index,
            properties: CardProperties::default(),
            derived_frames: None,
            steps: Vec::new(),
            step_index: 0,
            attachments: Vec::new(),
            lockout_type: ActionLockout::Animation,
            old_position: (0, 0),
            can_move_to_callback: None,
            update_callback: None,
            execute_callback: None,
            end_callback: None,
            animation_end_callback: None,
        }
    }

    pub fn create(
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        animation_state: String,
        entity_id: EntityId,
    ) -> Option<GenerationalIndex> {
        let entities = &mut simulation.entities;
        let Ok(entity) = entities.query_one_mut::<&mut Entity>(entity_id.into()) else {
            return None;
        };

        let sprite_tree = simulation.sprite_trees.get_mut(entity.sprite_tree_index)?;

        let mut sprite_node = SpriteNode::new(game_io, SpriteColorMode::Add);
        sprite_node.set_visible(false);

        let sprite_index = sprite_tree.insert_root_child(sprite_node);

        let action = Action::new(entity_id, animation_state, sprite_index);
        Some(simulation.actions.insert(action))
    }

    pub fn create_from_card_properties(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
        namespace: PackageNamespace,
        card_props: &CardProperties,
    ) -> Option<GenerationalIndex> {
        let package_id = &card_props.package_id;

        if package_id.is_blank() {
            return None;
        }

        let namespace = card_props.namespace.unwrap_or(namespace);

        let Ok(vm_index) = resources.vm_manager.find_vm(package_id, namespace) else {
            log::error!("Failed to find vm for {package_id} with namespace: {namespace:?}");
            return None;
        };

        let vms = resources.vm_manager.vms();
        let lua = &vms[vm_index].lua;
        let Ok(card_init) = lua.globals().get::<_, rollback_mlua::Function>("card_init") else {
            log::error!("{package_id} is missing card_init()");
            return None;
        };

        let api_ctx = RefCell::new(BattleScriptContext {
            vm_index,
            resources,
            game_io,
            simulation,
        });

        let lua_api = &game_io.resource::<Globals>().unwrap().battle_api;
        let mut id: Option<GenerationalIndex> = None;

        lua_api.inject_dynamic(lua, &api_ctx, |lua| {
            use rollback_mlua::IntoLua;

            // init card action
            let entity_table = create_entity_table(lua, entity_id)?;
            let lua_card_props = card_props.into_lua(lua)?;

            let optional_table = match card_init
                .call::<_, Option<rollback_mlua::Table>>((entity_table, lua_card_props))
            {
                Ok(optional_table) => optional_table,
                Err(err) => {
                    log::error!("{package_id}: {err}");
                    return Ok(());
                }
            };

            id = optional_table
                .map(|table| table.raw_get("#id"))
                .transpose()?;

            Ok(())
        });

        let index = id?;

        if let Some(action) = simulation.actions.get_mut(index) {
            // set card properties on the card action
            action.properties = card_props.clone();
        } else {
            return None;
        };

        Some(index)
    }

    pub fn is_async(&self) -> bool {
        matches!(self.lockout_type, ActionLockout::Async(_))
    }

    pub fn execute(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        action_index: GenerationalIndex,
    ) {
        let action = &mut simulation.actions[action_index];
        let entity_id = action.entity;

        let entity = simulation
            .entities
            .query_one_mut::<&mut Entity>(entity_id.into())
            .unwrap();

        action.executed = true;
        action.old_position = (entity.x, entity.y);

        // animations
        let animator_index = entity.animator_index;

        if let Some(derived_frames) = action.derived_frames.take() {
            action.state = BattleAnimator::derive_state(
                &mut simulation.animators,
                &action.state,
                derived_frames,
                animator_index,
            );
        }

        let animator = &mut simulation.animators[animator_index];

        if animator.has_state(&action.state) {
            let callbacks = animator.set_state(&action.state);
            simulation.pending_callbacks.extend(callbacks);

            // update entity sprite
            if let Some(sprite_tree) = simulation.sprite_trees.get_mut(entity.sprite_tree_index) {
                let sprite_node = sprite_tree.root_mut();
                animator.apply(sprite_node);
            }
        }

        // allow attacks to counter
        entity.hit_context.flags = HitFlag::NONE;

        // execute callback
        if let Some(callback) = action.execute_callback.take() {
            callback.call(game_io, resources, simulation, ());
        }

        // setup frame callbacks
        let Some(action) = simulation.actions.get_mut(action_index) else {
            return;
        };

        let animator = &mut simulation.animators[animator_index];

        for (frame_index, callback) in std::mem::take(&mut action.frame_callbacks) {
            animator.on_frame(frame_index, callback, false);
        }

        // animation end callback
        let animation_end_callback =
            BattleCallback::new(move |game_io, resources, simulation, _| {
                let Some(action) = simulation.actions.get_mut(action_index) else {
                    return;
                };

                if let Some(callback) = action.animation_end_callback.clone() {
                    callback.call(game_io, resources, simulation, ());
                }

                let Some(action) = simulation.actions.get_mut(action_index) else {
                    return;
                };

                if matches!(action.lockout_type, ActionLockout::Animation) {
                    Action::delete_multi(game_io, resources, simulation, [action_index]);
                }
            });

        animator.on_complete(animation_end_callback.clone());

        let interrupt_callback = BattleCallback::new(move |game_io, resources, simulation, _| {
            animation_end_callback.call(game_io, resources, simulation, ());
        });

        animator.on_interrupt(interrupt_callback);

        // update attachments
        let entity = simulation
            .entities
            .query_one_mut::<&mut Entity>(entity_id.into())
            .unwrap();

        if let Some(sprite_tree) = simulation.sprite_trees.get_mut(entity.sprite_tree_index) {
            if let Some(sprite) = sprite_tree.get_mut(action.sprite_index) {
                sprite.set_visible(true);
            }

            for attachment in &mut action.attachments {
                attachment.apply_animation(sprite_tree, &mut simulation.animators);
            }
        }
    }

    pub fn queue_action(
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
        index: GenerationalIndex,
    ) {
        let entities = &mut simulation.entities;
        let Ok((entity, character)) =
            entities.query_one_mut::<(&mut Entity, Option<&mut Character>)>(entity_id.into())
        else {
            return;
        };

        let Some(action) = simulation.actions.get(index) else {
            return;
        };

        let time_is_frozen = simulation.time_freeze_tracker.time_is_frozen();

        if time_is_frozen && action.properties.time_freeze {
            entity.action_queue.push_front(index);
        } else {
            entity.action_queue.push_back(index);
        }

        if let Some(character) = character {
            character.card_use_requested = false;
        }
    }

    pub fn queue_first_from_factories(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
        callbacks: Vec<BattleCallback<(), Option<GenerationalIndex>>>,
    ) {
        // resolve action from callbacks
        let action_index = callbacks
            .into_iter()
            .flat_map(|callback| callback.call(game_io, resources, simulation, ()))
            .next();

        let Some(action_index) = action_index else {
            // no action resolved
            return;
        };

        // queue action
        let entities = &mut simulation.entities;
        let Ok(entity) = entities.query_one_mut::<&mut Entity>(entity_id.into()) else {
            return;
        };

        entity.action_queue.push_back(action_index);
    }

    pub fn cancel_all(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
    ) {
        let entities = &mut simulation.entities;
        let Ok(entity) = entities.query_one_mut::<&mut Entity>(entity_id.into()) else {
            return;
        };

        let mut indices = std::mem::take(&mut entity.action_queue);

        if let Some(index) = entity.action_index {
            indices.push_back(index);
        }

        Action::delete_multi(game_io, resources, simulation, indices);
    }

    pub fn process_queues(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        let entities = &mut simulation.entities;

        // ensure initial test values for all action related aux props
        for (_, living) in entities.query_mut::<&mut Living>() {
            for aux_prop in living.aux_props.values_mut() {
                if aux_prop.effect().resolves_action() {
                    aux_prop.process_card(None);
                }
            }
        }

        // get a list of entity ids for entities that need processing
        let time_freeze_tracker = &simulation.time_freeze_tracker;
        let mut time_is_frozen = time_freeze_tracker.time_is_frozen();
        let mut any_can_counter_time_freeze = time_freeze_tracker.can_processing_action_counter();
        let polled_freezer = time_freeze_tracker.polled_entity();

        let ids: Vec<_> = entities
            .query_mut::<&Entity>()
            .into_iter()
            .filter(|(_, entity)| {
                let Some(action_index) = entity.action_queue.front() else {
                    // no actions queued
                    return false;
                };

                let Some(action) = simulation.actions.get(*action_index) else {
                    // remove this deleted action from the queue
                    log::error!("Queued Action deleted?");
                    return true;
                };

                // time freeze counter
                let freezes_time = action.properties.time_freeze;
                let action_counters_time_freeze =
                    freezes_time && !action.properties.skip_time_freeze_intro;
                let time_freeze_counter =
                    any_can_counter_time_freeze && action_counters_time_freeze;

                // continuing freeze from a previous action
                let freeze_continuation = freezes_time && polled_freezer == Some(entity.id);

                // we can't process an action if the entity is already in an action
                // unless we have a time freeze exception
                let already_has_action = entity.action_index.is_some();
                let can_process = (!time_is_frozen && !already_has_action)
                    || time_freeze_counter
                    || freeze_continuation;

                if can_process && freezes_time {
                    // causes other actions to wait in queue until time freeze is over
                    // or until countering is possible
                    time_is_frozen = true;
                    any_can_counter_time_freeze = false;
                }

                can_process
            })
            .map(|(id, _)| id)
            .collect();

        for id in ids {
            let entities = &mut simulation.entities;
            let Ok(entity) = entities.query_one_mut::<&mut Entity>(id) else {
                continue;
            };

            let Some(index) = entity.action_queue.pop_front() else {
                continue;
            };

            // aux props
            let Some(index) =
                Living::intercept_action(game_io, resources, simulation, id.into(), index)
            else {
                continue;
            };

            // validate index as it may be coming from lua
            let Some(action) = simulation.actions.get_mut(index) else {
                continue;
            };

            if action.processed {
                log::error!("Action already processed, ignoring");
                continue;
            }

            let entities = &mut simulation.entities;
            let entity = entities.query_one_mut::<&mut Entity>(id).unwrap();

            if action.entity != entity.id {
                continue;
            }

            action.processed = true;

            if action.properties.time_freeze {
                let time_freeze_tracker = &mut simulation.time_freeze_tracker;
                let dropped_action_index =
                    time_freeze_tracker.set_team_action(entity.team, index, action);

                if let Some(action_index) = dropped_action_index {
                    // delete previous action
                    Action::delete_multi(game_io, resources, simulation, [action_index]);
                }
            } else {
                entity.action_index = Some(index);
            }
        }

        // clean up action related aux props
        Living::aux_prop_cleanup(simulation, |aux_prop| aux_prop.effect().resolves_action());

        if simulation.time_freeze_tracker.time_is_frozen() {
            // cancel card_use_requested to fix cards used
            for (_, player) in simulation.entities.query_mut::<&mut Character>() {
                player.card_use_requested = false;
            }
        }
    }

    pub fn process_actions(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        let mut actions_pending_deletion = Vec::new();
        let time_is_frozen = simulation.time_freeze_tracker.time_is_frozen();

        let action_indices: Vec<_> = (simulation.actions)
            .iter()
            .filter(|(_, action)| action.processed)
            .map(|(index, _)| index)
            .collect();

        // card actions
        for action_index in action_indices {
            Living::attempt_interrupt(game_io, resources, simulation, action_index);

            let Some(action) = simulation.actions.get_mut(action_index) else {
                continue;
            };

            if time_is_frozen && !action.properties.time_freeze {
                // non time freeze action in time freeze
                continue;
            }

            let entities = &mut simulation.entities;
            let entity_id = action.entity;

            let Ok((entity, movement)) =
                entities.query_one_mut::<(&mut Entity, Option<&Movement>)>(entity_id.into())
            else {
                continue;
            };

            if !entity.spawned || entity.deleted || entity.time_frozen {
                continue;
            }

            let animator_index = entity.animator_index;

            // execute
            if !action.executed {
                if movement.is_some() {
                    continue;
                }

                if !simulation.is_entity_actionable(resources, entity_id) {
                    continue;
                }

                Action::execute(game_io, resources, simulation, action_index);
            }

            // update callback
            let Some(action) = simulation.actions.get_mut(action_index) else {
                continue;
            };

            if let Some(callback) = action.update_callback.clone() {
                callback.call(game_io, resources, simulation, ());
            }

            // steps
            let Some(action) = simulation.actions.get_mut(action_index) else {
                continue;
            };

            while action.step_index < action.steps.len() {
                let step = &mut action.steps[action.step_index];

                if !step.completed {
                    let callback = step.callback.clone();

                    callback.call(game_io, resources, simulation, ());
                    break;
                }

                action.step_index += 1;
            }

            // handling async card actions
            let Some(action) = simulation.actions.get_mut(action_index) else {
                continue;
            };

            let animation_completed = simulation.animators[animator_index].is_complete();

            let entity = simulation
                .entities
                .query_one_mut::<&mut Entity>(action.entity.into())
                .unwrap();

            if action.is_async() && animation_completed && entity.action_index == Some(action_index)
            {
                // async action completed sync portion
                action.complete_sync(
                    &mut simulation.entities,
                    &mut simulation.pending_callbacks,
                    &mut simulation.field,
                );
            }

            // detecting end
            let is_complete = match action.lockout_type {
                ActionLockout::Animation => animation_completed,
                ActionLockout::Sequence => action.step_index >= action.steps.len(),
                ActionLockout::Async(frames) => action.active_frames >= frames,
            };

            action.active_frames += 1;

            if is_complete {
                // queue deletion
                actions_pending_deletion.push(action_index);
            }
        }

        Living::aux_prop_cleanup(simulation, |aux_prop| {
            aux_prop.effect().executes_on_current_action()
        });

        Action::delete_multi(game_io, resources, simulation, actions_pending_deletion);
    }

    pub fn delete_multi(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        delete_indices: impl IntoIterator<Item = GenerationalIndex>,
    ) {
        for index in delete_indices {
            let Some(action) = simulation.actions.get_mut(index) else {
                continue;
            };

            if action.deleted {
                // avoid callbacks calling delete_actions on this card action
                continue;
            }

            action.deleted = true;

            // remove the index from the entity
            let entity = simulation
                .entities
                .query_one_mut::<&mut Entity>(action.entity.into())
                .unwrap();

            if entity.action_index == Some(index) {
                action.complete_sync(
                    &mut simulation.entities,
                    &mut simulation.pending_callbacks,
                    &mut simulation.field,
                );
            }

            // end callback
            if let Some(callback) = action.end_callback.clone() {
                callback.call(game_io, resources, simulation, ());
            }

            let action = simulation.actions.get(index).unwrap();

            // remove attachments from the entity
            let entity = simulation
                .entities
                .query_one_mut::<&mut Entity>(action.entity.into())
                .unwrap();

            if let Some(sprite_tree) = simulation.sprite_trees.get_mut(entity.sprite_tree_index) {
                sprite_tree.remove(action.sprite_index);
            }

            for attachment in &action.attachments {
                simulation.animators.remove(attachment.animator_index);
            }

            // finally remove the card action
            simulation.actions.remove(index);
        }

        simulation.call_pending_callbacks(game_io, resources);
    }

    fn complete_sync(
        &mut self,
        entities: &mut hecs::World,
        pending_callbacks: &mut Vec<BattleCallback>,
        field: &mut Field,
    ) {
        let entity_id = self.entity.into();
        let entity = entities.query_one_mut::<&mut Entity>(entity_id).unwrap();

        // unset action_index to allow other card actions to be used
        entity.action_index = None;

        // update reservations as they're ignored while in a sync card action
        if self.executed && entity.auto_reserves_tiles {
            let old_tile = field.tile_at_mut(self.old_position).unwrap();
            old_tile.remove_reservation_for(entity.id);

            let current_tile = field.tile_at_mut((entity.x, entity.y)).unwrap();
            current_tile.reserve_for(entity.id);
        }

        pending_callbacks.push(entity.idle_callback.clone());
    }
}

#[derive(Clone)]
pub struct ActionAttachment {
    pub point_name: String,
    pub sprite_index: GenerationalIndex,
    pub animator_index: GenerationalIndex,
    pub parent_animator_index: GenerationalIndex,
}

impl ActionAttachment {
    pub fn new(
        point_name: String,
        sprite_index: GenerationalIndex,
        animator_index: GenerationalIndex,
        parent_animator_index: GenerationalIndex,
    ) -> Self {
        Self {
            point_name,
            sprite_index,
            animator_index,
            parent_animator_index,
        }
    }

    pub fn apply_animation(
        &self,
        sprite_tree: &mut Tree<SpriteNode>,
        animators: &mut SlotMap<BattleAnimator>,
    ) {
        let Some(sprite_node) = sprite_tree.get_mut(self.sprite_index) else {
            return;
        };

        let animator = &mut animators[self.animator_index];
        animator.enable();
        animator.apply(sprite_node);

        // attach to point
        let parent_animator = &mut animators[self.parent_animator_index];

        if let Some(point) = parent_animator.point(&self.point_name) {
            sprite_node.set_offset(point - parent_animator.origin());
            sprite_node.set_visible(true);
        } else {
            sprite_node.set_visible(false);
        }
    }
}

#[derive(Clone, Default)]
pub struct ActionStep {
    pub completed: bool,
    pub callback: BattleCallback,
}
