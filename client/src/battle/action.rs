use super::{
    BattleAnimator, BattleCallback, BattleScriptContext, BattleSimulation, Entity, Field,
    SharedBattleResources,
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

        // set card properties on the card action
        if let Some(index) = id {
            if let Some(action) = simulation.actions.get_mut(index) {
                action.properties = card_props.clone();
            }
        }

        id
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
                    simulation.delete_actions(game_io, resources, &[action_index]);
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
