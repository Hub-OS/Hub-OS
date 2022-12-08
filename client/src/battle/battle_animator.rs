use super::{BattleCallback, Entity};
use crate::bindable::{AnimatorPlaybackMode, EntityID, GenerationalIndex};
use crate::render::{
    Animator, AnimatorLoopMode, DerivedFrame, DerivedState, FrameTime, SpriteNode,
};
use crate::resources::Globals;
use framework::prelude::{GameIO, Vec2};
use std::collections::HashMap;

#[derive(Clone)]
pub struct BattleAnimator {
    target: Option<(hecs::Entity, GenerationalIndex)>,
    synced_animators: Vec<generational_arena::Index>,
    complete_callbacks: Vec<BattleCallback>,
    interrupt_callbacks: Vec<BattleCallback>,
    frame_callbacks: HashMap<usize, Vec<(BattleCallback, bool)>>,
    animator: Animator,
    time: FrameTime,
    enabled: bool,
}

impl BattleAnimator {
    pub fn new() -> Self {
        Self {
            target: None,
            synced_animators: Vec::new(),
            complete_callbacks: Vec::new(),
            interrupt_callbacks: Vec::new(),
            frame_callbacks: HashMap::new(),
            animator: Animator::new(),
            time: 0,
            enabled: true,
        }
    }

    pub fn set_target(&mut self, entity_id: EntityID, sprite_index: GenerationalIndex) {
        self.target = Some((entity_id.into(), sprite_index));
    }

    pub fn target_entity_id(&self) -> Option<EntityID> {
        self.target.map(|(entity_id, _)| entity_id.into())
    }

    pub fn target_sprite_index(&self) -> Option<GenerationalIndex> {
        self.target.map(|(_, sprite_index)| sprite_index)
    }

    pub fn find_and_apply_to_target(&self, entities: &mut hecs::World) {
        if let Some(sprite_node) = self.find_target(entities) {
            self.apply(sprite_node)
        }
    }

    fn find_target<'a>(&self, entities: &'a mut hecs::World) -> Option<&'a mut SpriteNode> {
        let (entity_id, sprite_index) = self.target?;
        let entity = entities.query_one_mut::<&mut Entity>(entity_id).ok()?;

        entity.sprite_tree.get_mut(sprite_index)
    }

    pub fn has_synced_animators(&self) -> bool {
        !self.synced_animators.is_empty()
    }

    pub fn add_synced_animator(&mut self, animator_index: generational_arena::Index) {
        self.synced_animators.push(animator_index);
    }

    pub fn remove_synced_animator(&mut self, animator_index: generational_arena::Index) {
        if let Some(index) = (self.synced_animators.iter()).position(|i| *i == animator_index) {
            self.synced_animators.remove(index);
        }
    }

    pub fn sync_animators(
        battle_animators: &mut generational_arena::Arena<BattleAnimator>,
        entities: &mut hecs::World,
        pending_callbacks: &mut Vec<BattleCallback>,
        animator_index: generational_arena::Index,
    ) {
        let Some(battle_animator) = battle_animators.get(animator_index) else {
            return;
        };

        let Some(state) = battle_animator.current_state().map(|state| state.to_string()) else {
            return;
        };

        let time = battle_animator.time;
        let loop_mode = battle_animator.animator.loop_mode();
        let reversed = battle_animator.animator.reversed();

        for index in battle_animator.synced_animators.clone() {
            let battle_animator = &mut battle_animators[index];

            battle_animator.sync(pending_callbacks, &state, loop_mode, reversed, time);

            battle_animator.find_and_apply_to_target(entities);
        }
    }

    pub fn derived_states(&self) -> &[DerivedState] {
        self.animator.derived_states()
    }

    pub fn copy_derive_states(&mut self, derived_states: Vec<DerivedState>) {
        for derived_state in derived_states {
            self.animator.derive_state(
                &derived_state.state,
                &derived_state.original_state,
                derived_state.frame_derivation,
            );
        }
    }

    pub fn derive_state(
        battle_animators: &mut generational_arena::Arena<BattleAnimator>,
        original_state: &str,
        frame_derivation: Vec<DerivedFrame>,
        animator_index: generational_arena::Index,
    ) -> String {
        let new_state = Animator::generate_state_id(original_state);

        let battle_animator = &mut battle_animators[animator_index];

        (battle_animator.animator).derive_state(
            &new_state,
            original_state,
            frame_derivation.clone(),
        );

        for index in battle_animator.synced_animators.clone() {
            let battle_animator = &mut battle_animators[index];
            (battle_animator.animator).derive_state(
                &new_state,
                original_state,
                frame_derivation.clone(),
            );
        }

        new_state
    }

    pub fn disable(&mut self) {
        self.enabled = false;
    }

    pub fn enable(&mut self) {
        self.enabled = true;
    }

    pub fn clear_callbacks(&mut self) {
        self.complete_callbacks.clear();
        self.frame_callbacks.clear();
        self.interrupt_callbacks.clear();
    }

    #[must_use]
    pub fn load(&mut self, game_io: &GameIO<Globals>, path: &str) -> Vec<BattleCallback> {
        let had_state = self.animator.current_state().is_some();
        let completed = self.animator.is_complete();

        self.animator.load(&game_io.globals().assets, path);
        self.animator.sync_time(self.time);

        if !completed && had_state && self.animator.current_state().is_none() {
            // lost state, treat as interrupted
            self.complete_callbacks.clear();
            self.frame_callbacks.clear();

            std::mem::take(&mut self.interrupt_callbacks)
        } else {
            // state retained
            Vec::new()
        }
    }

    #[must_use]
    pub fn copy_from(&mut self, other: &Self) -> Vec<BattleCallback> {
        self.animator.copy_from(&other.animator);

        if let Some(state) = self.current_state() {
            // activate interrupt callbacks and clear other listeners by resetting state
            // resets progress as well
            self.set_state(&state.to_string())
        } else {
            Vec::new()
        }
    }

    pub fn on_complete(&mut self, callback: BattleCallback) {
        self.complete_callbacks.push(callback);
    }

    pub fn on_interrupt(&mut self, callback: BattleCallback) {
        self.interrupt_callbacks.push(callback);
    }

    pub fn on_frame(&mut self, frame_index: usize, callback: BattleCallback, do_once: bool) {
        if let Some(callbacks) = self.frame_callbacks.get_mut(&frame_index) {
            callbacks.push((callback, do_once));
        } else {
            self.frame_callbacks
                .insert(frame_index, vec![(callback, do_once)]);
        }
    }

    pub fn set_loop_mode(&mut self, mode: AnimatorLoopMode) {
        self.animator.set_loop_mode(mode)
    }

    pub fn set_playback_mode(&mut self, mode: AnimatorPlaybackMode) {
        match mode {
            AnimatorPlaybackMode::Once => {
                self.animator.set_loop_mode(AnimatorLoopMode::Once);
                self.animator.set_reversed(false);
            }
            AnimatorPlaybackMode::Loop => {
                self.animator.set_loop_mode(AnimatorLoopMode::Loop);
                self.animator.set_reversed(false);
            }
            AnimatorPlaybackMode::Bounce => {
                self.animator.set_loop_mode(AnimatorLoopMode::Bounce);
                self.animator.set_reversed(false);
            }
            AnimatorPlaybackMode::Reverse => {
                self.animator.set_loop_mode(AnimatorLoopMode::Once);
                self.animator.set_reversed(true);
            }
        }
    }

    pub fn set_reversed(&mut self, reversed: bool) {
        self.animator.set_reversed(reversed)
    }

    pub fn origin(&self) -> Vec2 {
        self.animator.origin()
    }

    pub fn point(&self, name: &str) -> Option<Vec2> {
        self.animator.point(name)
    }

    pub fn is_complete(&self) -> bool {
        self.animator.is_complete()
    }

    pub fn current_state(&self) -> Option<&str> {
        self.animator.current_state()
    }

    pub fn has_state(&self, state: &str) -> bool {
        self.animator.has_state(state)
    }

    #[must_use]
    pub fn set_state(&mut self, state: &str) -> Vec<BattleCallback> {
        self.animator.set_state(state);
        self.time = 0;

        self.complete_callbacks.clear();
        self.frame_callbacks.clear();

        std::mem::take(&mut self.interrupt_callbacks)
    }

    #[must_use]
    pub fn update(&mut self) -> Vec<BattleCallback> {
        let mut pending_callbacks = Vec::new();

        self.internal_update(&mut pending_callbacks, |battle_animator| {
            battle_animator.animator.update();
            battle_animator.time += 1;
        });

        pending_callbacks
    }

    fn sync(
        &mut self,
        pending_callbacks: &mut Vec<BattleCallback>,
        state: &str,
        loop_mode: AnimatorLoopMode,
        reversed: bool,
        time: FrameTime,
    ) {
        let state_differs = self.current_state() != Some(&state);

        if state_differs {
            pending_callbacks.extend(self.set_state(&state));
        }

        let resync_required =
            self.animator.loop_mode() != loop_mode || self.animator.reversed() != reversed;

        self.animator.set_loop_mode(loop_mode);
        self.animator.set_reversed(reversed);

        if !resync_required {
            if time - self.time == 1 {
                // avoid complex sync logic if we only need to advance one frame
                pending_callbacks.extend(self.update());
                self.time = time;
                return;
            }

            if self.time == time {
                return;
            }
        }

        if time == 0 {
            // avoid call_frame_callbacks for time == 0
            // simpler internal_update
            let previous_frame = self.animator.current_frame_index();

            self.animator.sync_time(0);
            self.time = 0;

            let current_frame = self.animator.current_frame_index();

            if previous_frame != current_frame {
                self.call_frame_callbacks(pending_callbacks, current_frame);
            }

            if self.animator.is_complete() {
                pending_callbacks.extend(self.complete_callbacks.clone());
                self.interrupt_callbacks.clear();
            }

            return;
        }

        self.internal_update(pending_callbacks, |battle_animator| {
            battle_animator.animator.sync_time(time);
            battle_animator.time = time;
        });
    }

    fn internal_update(
        &mut self,
        pending_callbacks: &mut Vec<BattleCallback>,
        update: impl FnOnce(&mut Self),
    ) {
        if !self.enabled || self.animator.is_complete() {
            return;
        }

        let previous_frame = self.animator.current_frame_index();
        let previous_loop_count = self.animator.loop_count();

        if self.time == 0 {
            self.call_frame_callbacks(pending_callbacks, previous_frame);
        }

        update(self);

        let current_frame = self.animator.current_frame_index();

        if previous_frame != current_frame {
            self.call_frame_callbacks(pending_callbacks, current_frame);
        }

        if self.animator.is_complete() || previous_loop_count != self.animator.loop_count() {
            pending_callbacks.extend(self.complete_callbacks.clone());
        }

        if self.animator.is_complete() {
            self.interrupt_callbacks.clear();
        }
    }

    fn call_frame_callbacks(&mut self, pending_callbacks: &mut Vec<BattleCallback>, frame: usize) {
        if let Some(callbacks) = self.frame_callbacks.get_mut(&frame) {
            pending_callbacks.extend(callbacks.iter().map(|(callback, _)| callback.clone()));

            let pending_removal: Vec<_> = callbacks
                .iter()
                .enumerate()
                .filter(|(_, (_, do_once))| *do_once)
                .map(|(i, _)| i)
                .rev()
                .collect();

            for index in pending_removal {
                callbacks.remove(index);
            }
        }
    }

    pub fn apply(&self, sprite_node: &mut SpriteNode) {
        sprite_node.apply_animation(&self.animator);
    }
}
