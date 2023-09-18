use super::{
    BattleAnimator, BattleCallback, BattleScriptContext, BattleSimulation, Character, Entity,
    Field, Living, SharedBattleResources,
};
use crate::bindable::{ActionLockout, CardProperties, EntityId, GenerationalIndex, HitFlag};
use crate::lua_api::create_entity_table;
use crate::packages::PackageNamespace;
use crate::render::{AnimatorLoopMode, DerivedFrame, FrameTime, SpriteNode, Tree};
use crate::resources::Globals;
use crate::structures::SlotMap;
use framework::prelude::GameIO;
use std::cell::RefCell;

#[derive(Clone)]
pub struct Action {
    pub active_frames: FrameTime,
    pub deleted: bool,
    pub executed: bool,
    pub used: bool,
    pub entity: EntityId,
    pub state: String,
    pub prev_state: Option<(String, AnimatorLoopMode, bool)>,
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
    pub fn new(entity_id: EntityId, state: String, sprite_index: GenerationalIndex) -> Self {
        Self {
            active_frames: 0,
            deleted: false,
            executed: false,
            used: false,
            entity: entity_id,
            state,
            prev_state: None,
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

        let Ok(vm_index) = resources.vm_manager.find_vm(package_id, namespace) else {
            log::error!("Failed to find vm for {package_id}");
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

        let Some(index) = id else {
            return None;
        };

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

        // animations
        let animator_index = entity.animator_index;
        let animator = &mut simulation.animators[animator_index];

        action.prev_state = animator
            .current_state()
            .map(|state| (state.to_string(), animator.loop_mode(), animator.reversed()));

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
            let sprite_node = entity.sprite_tree.root_mut();
            animator.apply(sprite_node);
        }

        // allow attacks to counter
        let original_context_flags = entity.hit_context.flags;
        entity.hit_context.flags = HitFlag::NONE;

        // execute callback
        if let Some(callback) = action.execute_callback.take() {
            callback.call(game_io, resources, simulation, ());
        }

        let entity = simulation
            .entities
            .query_one_mut::<&mut Entity>(entity_id.into())
            .unwrap();

        // revert context
        entity.hit_context.flags = original_context_flags;

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
        if let Some(sprite) = entity.sprite_tree.get_mut(action.sprite_index) {
            sprite.set_visible(true);
        }

        for attachment in &mut action.attachments {
            attachment.apply_animation(&mut entity.sprite_tree, &mut simulation.animators);
        }

        action.executed = true;
        action.old_position = (entity.x, entity.y);
    }

    pub fn queue_action(
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
        index: GenerationalIndex,
    ) {
        let can_counter = simulation.time_freeze_tracker.can_counter();

        let entities = &mut simulation.entities;
        let Ok(entity) = entities.query_one_mut::<&mut Entity>(entity_id.into()) else {
            return;
        };

        let Some(action) = simulation.actions.get(index) else {
            return;
        };

        if can_counter && action.properties.time_freeze {
            entity.action_queue.push_front(index);
        } else {
            entity.action_queue.push_back(index);
        }

        if let Ok(character) = entities.query_one_mut::<&mut Character>(entity_id.into()) {
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
        let time_is_frozen = simulation.time_freeze_tracker.time_is_frozen();
        let can_counter = simulation.time_freeze_tracker.can_queued_counter();

        let entities = &mut simulation.entities;

        // ensure initial test values for all action related aux props
        for (_, living) in entities.query_mut::<&mut Living>() {
            for aux_prop in living.aux_props.values_mut() {
                if aux_prop.effect().action_related() {
                    aux_prop.process_action(None);
                }
            }
        }

        // get a list of entity ids for entities that need processing
        let ids: Vec<_> = entities
            .query_mut::<&Entity>()
            .into_iter()
            .filter(|(_, entity)| {
                let already_has_action = !time_is_frozen && entity.action_index.is_some();
                let time_freeze_counter = can_counter
                    && entity
                        .action_queue
                        .front()
                        .and_then(|index| simulation.actions.get(*index))
                        .map(|action| action.properties.time_freeze)
                        .unwrap_or_default();

                let has_pending_actions = !entity.action_queue.is_empty();

                (!already_has_action || time_freeze_counter) && has_pending_actions
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

            if action.used {
                log::error!("Action already used, ignoring");
                continue;
            }

            let entities = &mut simulation.entities;
            let entity = entities.query_one_mut::<&mut Entity>(id).unwrap();

            if action.entity != entity.id {
                continue;
            }

            action.used = true;

            if action.properties.time_freeze {
                if can_counter && !simulation.is_resimulation {
                    // must be countering, play sfx
                    let globals = game_io.resource::<Globals>().unwrap();
                    globals.audio.play_sound(&globals.sfx.trap);
                }

                let time_freeze_tracker = &mut simulation.time_freeze_tracker;
                time_freeze_tracker.set_team_action(entity.team, index);
            } else {
                entity.action_index = Some(index);
            }
        }

        // clean up action related aux props
        let entities = &mut simulation.entities;

        for (_, living) in entities.query_mut::<&mut Living>() {
            for aux_prop in living.aux_props.values_mut() {
                if aux_prop.effect().action_related() {
                    aux_prop.mark_tested();
                }
            }

            living.delete_completed_aux_props();

            for aux_prop in living.aux_props.values_mut() {
                if aux_prop.effect().action_related() {
                    aux_prop.reset_tests();
                }
            }
        }

        if simulation.time_freeze_tracker.time_is_frozen() {
            // cancel card_use_requested to fix cards used
            for (_, player) in entities.query_mut::<&mut Character>() {
                player.card_use_requested = false;
            }
        }
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
                    &mut simulation.animators,
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

            entity.sprite_tree.remove(action.sprite_index);

            for attachment in &action.attachments {
                simulation.animators.remove(attachment.animator_index);
            }

            // finally remove the card action
            simulation.actions.remove(index);
        }

        simulation.call_pending_callbacks(game_io, resources);
    }

    pub fn complete_sync(
        &mut self,
        entities: &mut hecs::World,
        animators: &mut SlotMap<BattleAnimator>,
        pending_callbacks: &mut Vec<BattleCallback>,
        field: &mut Field,
    ) {
        let entity_id = self.entity.into();
        let entity = entities.query_one_mut::<&mut Entity>(entity_id).unwrap();

        // unset action_index to allow other card actions to be used
        entity.action_index = None;

        // revert animation
        if let Some((state, loop_mode, reversed)) = self.prev_state.take() {
            let animator = &mut animators[entity.animator_index];
            let callbacks = animator.set_state(&state);
            animator.set_loop_mode(loop_mode);
            animator.set_reversed(reversed);

            pending_callbacks.extend(callbacks);

            let sprite_node = entity.sprite_tree.root_mut();
            animator.apply(sprite_node);
        }

        // update reservations as they're ignored while in a sync card action
        if entity.auto_reserves_tiles {
            let old_tile = field.tile_at_mut(self.old_position).unwrap();
            old_tile.remove_reservation_for(entity.id);

            let current_tile = field.tile_at_mut((entity.x, entity.y)).unwrap();
            current_tile.reserve_for(entity.id);
        }
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
        let sprite_node = match sprite_tree.get_mut(self.sprite_index) {
            Some(sprite_node) => sprite_node,
            None => return,
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
