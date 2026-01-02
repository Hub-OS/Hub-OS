use super::{BattleCallback, DerivedAnimationFrame, DerivedAnimationState};
use crate::bindable::{AnimatorPlaybackMode, GenerationalIndex};
use crate::render::{Animator, AnimatorLoopMode, FrameList, FrameTime, SpriteNode};
use crate::resources::Globals;
use crate::structures::{SlotMap, Tree, TreeIndex};
use framework::prelude::{GameIO, Vec2};
use std::collections::HashMap;

#[derive(Clone)]
pub struct BattleAnimator {
    // (tree_slot_index, sprite_index)
    target: Option<(GenerationalIndex, TreeIndex)>,
    derived_states: Vec<DerivedAnimationState>,
    synced_animators: Vec<GenerationalIndex>,
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
            derived_states: Vec::new(),
            synced_animators: Vec::new(),
            complete_callbacks: Vec::new(),
            interrupt_callbacks: Vec::new(),
            frame_callbacks: HashMap::new(),
            animator: Animator::new(),
            time: 0,
            enabled: true,
        }
    }

    pub fn animator(&self) -> &Animator {
        &self.animator
    }

    pub fn take_animator(self) -> Animator {
        self.animator
    }

    pub fn set_target(&mut self, slot_index: GenerationalIndex, sprite_index: TreeIndex) {
        self.target = Some((slot_index, sprite_index));
    }

    pub fn target_tree_index(&self) -> Option<GenerationalIndex> {
        self.target.map(|(slot_index, _)| slot_index)
    }

    pub fn target_sprite_index(&self) -> Option<TreeIndex> {
        self.target.map(|(_, sprite_index)| sprite_index)
    }

    pub fn find_and_apply_to_target(&self, sprite_trees: &mut SlotMap<Tree<SpriteNode>>) {
        if let Some(sprite_node) = self.find_target(sprite_trees) {
            self.apply(sprite_node)
        }
    }

    fn find_target<'a>(
        &self,
        sprite_trees: &'a mut SlotMap<Tree<SpriteNode>>,
    ) -> Option<&'a mut SpriteNode> {
        let (slot_index, sprite_index) = self.target?;

        sprite_trees.get_mut(slot_index)?.get_mut(sprite_index)
    }

    pub fn has_synced_animators(&self) -> bool {
        !self.synced_animators.is_empty()
    }

    pub fn synced_animators(&self) -> &[GenerationalIndex] {
        &self.synced_animators
    }

    pub fn add_synced_animator(&mut self, animator_index: GenerationalIndex) {
        self.synced_animators.push(animator_index);
    }

    pub fn remove_synced_animator(&mut self, animator_index: GenerationalIndex) {
        if let Some(index) = (self.synced_animators.iter()).position(|i| *i == animator_index) {
            self.synced_animators.remove(index);
        }
    }

    pub fn sync_animators(
        battle_animators: &mut SlotMap<BattleAnimator>,
        sprite_trees: &mut SlotMap<Tree<SpriteNode>>,
        pending_callbacks: &mut Vec<BattleCallback>,
        animator_index: GenerationalIndex,
    ) {
        let Some(battle_animator) = battle_animators.get(animator_index) else {
            return;
        };

        let state = battle_animator.current_state();
        let Some(state) = state.map(|state| state.to_string()) else {
            return;
        };

        let time = battle_animator.time;
        let loop_mode = battle_animator.animator.loop_mode();
        let reversed = battle_animator.animator.reversed();

        for index in battle_animator.synced_animators.clone() {
            let battle_animator = &mut battle_animators[index];

            battle_animator.sync(pending_callbacks, &state, loop_mode, reversed, time);

            battle_animator.find_and_apply_to_target(sprite_trees);
        }
    }

    pub fn derived_states(&self) -> &[DerivedAnimationState] {
        &self.derived_states
    }

    pub fn copy_derive_states(&mut self, derived_states: Vec<DerivedAnimationState>) {
        for derived_state in &derived_states {
            derived_state.apply(&mut self.animator);
        }

        self.derived_states.extend(derived_states);
    }

    pub fn derive_state(
        battle_animators: &mut SlotMap<BattleAnimator>,
        original_state: &str,
        frame_derivation: Vec<DerivedAnimationFrame>,
        animator_index: GenerationalIndex,
    ) -> String {
        let derived_state = DerivedAnimationState::new(original_state, frame_derivation);
        let new_state = derived_state.state.clone();

        let battle_animator = &mut battle_animators[animator_index];

        // apply to synced animators first to avoid an extra clone
        for index in battle_animator.synced_animators.clone() {
            let battle_animator = &mut battle_animators[index];
            derived_state.apply(&mut battle_animator.animator);
            battle_animator.derived_states.push(derived_state.clone());
        }

        // apply to the primary animator
        let battle_animator = &mut battle_animators[animator_index];
        derived_state.apply(&mut battle_animator.animator);
        battle_animator.derived_states.push(derived_state);

        new_state
    }

    fn remove_derived_state(&mut self, state: &str) {
        if let Some(i) = self.derived_states.iter().position(|s| s.state == state) {
            self.derived_states.swap_remove(i);
        }
    }

    pub fn remove_state(
        battle_animators: &mut SlotMap<BattleAnimator>,
        pending_callbacks: &mut Vec<BattleCallback>,
        state: &str,
        animator_index: GenerationalIndex,
    ) {
        let Some(battle_animator) = battle_animators.get_mut(animator_index) else {
            return;
        };

        if battle_animator.current_state() == Some(state) {
            pending_callbacks.extend(battle_animator.interrupt());
        }

        battle_animator.remove_derived_state(state);
        battle_animator.animator.remove_state(state);

        // remove from synced animators first to avoid an extra clone
        for index in battle_animator.synced_animators.clone() {
            if let Some(battle_animator) = battle_animators.get_mut(index) {
                if battle_animator.current_state() == Some(state) {
                    pending_callbacks.extend(battle_animator.interrupt());
                }

                battle_animator.remove_derived_state(state);
                battle_animator.animator.remove_state(state);
            }
        }
    }

    #[must_use]
    pub fn set_temp_derived_state(
        battle_animators: &mut SlotMap<BattleAnimator>,
        original_state: &str,
        frame_derivation: Vec<DerivedAnimationFrame>,
        animator_index: GenerationalIndex,
    ) -> Vec<BattleCallback> {
        let state = Self::derive_state(
            battle_animators,
            original_state,
            frame_derivation,
            animator_index,
        );

        let battle_animator = &mut battle_animators[animator_index];
        let callbacks = battle_animator.set_state(&state);

        let complete_callback = BattleCallback::new(move |_, _, simulation, _| {
            let Some(battle_animator) = simulation.animators.get_mut(animator_index) else {
                return;
            };

            if battle_animator.animator.current_state() == Some(&state)
                && !battle_animator.is_complete()
            {
                // ignore Playback.Loop
                // an interrupt would have a different state
                // a true completion would be marked as complete
                return;
            }

            Self::remove_state(
                &mut simulation.animators,
                &mut simulation.pending_callbacks,
                &state,
                animator_index,
            );
        });

        battle_animator.on_complete(complete_callback.clone());
        battle_animator.on_interrupt(complete_callback);

        callbacks
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
    pub fn load(&mut self, game_io: &GameIO, path: &str) -> Vec<BattleCallback> {
        let completed = self.animator.is_complete();
        let old_state = self.animator.current_state().map(|s| s.to_string());
        let loop_mode = self.animator.loop_mode();
        let reversed = self.animator.reversed();

        let assets = &Globals::from_resources(game_io).assets;
        self.animator.load(assets, path);

        // rederive states
        for derived_state in &self.derived_states {
            derived_state.apply(&mut self.animator);
        }

        if let Some(state) = &old_state {
            // reapply old settings
            self.animator.set_state(state);
            self.animator.set_loop_mode(loop_mode);
            self.animator.set_reversed(reversed);
            self.animator.sync_time(self.time);
        }

        if !completed && old_state.is_some() && self.animator.current_state().is_none() {
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

    #[must_use]
    pub fn copy_from_animator(&mut self, animator: &Animator) -> Vec<BattleCallback> {
        self.animator.copy_from(animator);

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

    pub fn loop_mode(&self) -> AnimatorLoopMode {
        self.animator.loop_mode()
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

        self.animator.sync_time(self.time);
    }

    pub fn reversed(&self) -> bool {
        self.animator.reversed()
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

    pub fn iter_states(&self) -> impl Iterator<Item = (&str, &FrameList)> {
        self.animator.iter_states()
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
        self.interrupt()
    }

    #[must_use]
    fn interrupt(&mut self) -> Vec<BattleCallback> {
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

    #[must_use]
    pub fn sync_time(&mut self, time: FrameTime) -> Vec<BattleCallback> {
        let mut pending_callbacks = Vec::new();

        self.internal_update(&mut pending_callbacks, |battle_animator| {
            battle_animator.animator.sync_time(time);
            battle_animator.time = time;
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
        let state_differs = self.current_state() != Some(state);

        if state_differs {
            pending_callbacks.extend(self.set_state(state));
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

            if self.animator.is_complete() {
                pending_callbacks.extend(self.complete_callbacks.clone());
                self.interrupt_callbacks.clear();
            }

            if previous_frame != current_frame {
                self.call_frame_callbacks(pending_callbacks, current_frame);
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

        if self.animator.is_complete() || previous_loop_count != self.animator.loop_count() {
            pending_callbacks.extend(self.complete_callbacks.iter().cloned());
        }

        if previous_frame != current_frame {
            self.call_frame_callbacks(pending_callbacks, current_frame);
        }

        if self.animator.is_complete() {
            self.interrupt_callbacks.clear();
        }
    }

    fn call_frame_callbacks(&mut self, pending_callbacks: &mut Vec<BattleCallback>, frame: usize) {
        if let Some(callbacks) = self.frame_callbacks.get_mut(&frame) {
            pending_callbacks.extend(callbacks.iter().map(|(callback, _)| callback.clone()));
            callbacks.retain(|(_, do_once)| !*do_once);
        }
    }

    pub fn apply(&self, sprite_node: &mut SpriteNode) {
        sprite_node.apply_animation(&self.animator);
    }
}
