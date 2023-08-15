use super::{BattleCallback, Entity, PlayerInput, SharedBattleResources, StatusRegistry};
use crate::bindable::{HitFlag, HitFlags, SpriteColorMode};
use crate::render::{AnimatorLoopMode, FrameTime, SpriteNode, TreeIndex};
use crate::resources::{
    Input, DEFAULT_INTANGIBILITY_DURATION, DEFAULT_STATUS_DURATION, DRAG_LOCKOUT,
};
use framework::prelude::GameIO;

const MASHABLE_STATUSES: [HitFlags; 2] = [HitFlag::PARALYZE, HitFlag::FREEZE];

#[derive(Clone)]
struct AppliedStatus {
    status_flag: HitFlags,
    remaining_time: FrameTime,
    lifetime: FrameTime,
    destructor: Option<BattleCallback>,
}

#[derive(Clone, Default)]
pub struct StatusDirector {
    statuses: Vec<AppliedStatus>,
    new_statuses: Vec<AppliedStatus>,
    ready_destructors: Vec<BattleCallback>,
    status_sprites: Vec<(HitFlags, TreeIndex)>,
    input_index: Option<usize>,
    dragged: bool,
    remaining_drag_lockout: FrameTime,
    remaining_shake_time: FrameTime,
}

impl StatusDirector {
    pub fn clear_statuses(&mut self) {
        self.ready_destructors.extend(
            std::mem::take(&mut self.statuses)
                .into_iter()
                .flat_map(|status| status.destructor),
        );

        self.new_statuses.clear();
        self.dragged = false;
        self.remaining_drag_lockout = 0;
        self.remaining_shake_time = 0;
    }

    pub fn set_input_index(&mut self, input_index: usize) {
        self.input_index = Some(input_index);
    }

    pub fn set_destructor(&mut self, flag: HitFlags, destructor: Option<BattleCallback>) {
        let Some(status) = self
            .statuses
            .iter_mut()
            .find(|status| status.status_flag == flag)
        else {
            return;
        };

        status.destructor = destructor;
    }

    pub fn apply_hit_flags(&mut self, status_registry: &StatusRegistry, hit_flags: HitFlags) {
        for hit_flag in HitFlag::BUILT_IN {
            if hit_flags & hit_flag == HitFlag::NONE {
                continue;
            }

            if hit_flag == HitFlag::DRAG {
                self.dragged = true;
            }

            let duration = if HitFlag::BUILT_IN_STATUSES.contains(&hit_flag) {
                DEFAULT_STATUS_DURATION
            } else if hit_flag == HitFlag::FLASH {
                DEFAULT_INTANGIBILITY_DURATION
            } else {
                0
            };

            if hit_flag == HitFlag::SHAKE {
                self.remaining_shake_time = DEFAULT_STATUS_DURATION;
            }

            self.apply_status(hit_flag, duration);
        }

        for registered_status in status_registry.registered_list() {
            if hit_flags & registered_status.flag == HitFlag::NONE {
                continue;
            }

            self.apply_status(registered_status.flag, registered_status.durations[0]);
        }
    }

    pub fn apply_status(&mut self, status_flag: HitFlags, duration: FrameTime) {
        let status_search = self
            .new_statuses
            .iter_mut()
            .find(|status| status.status_flag == status_flag);

        if let Some(status) = status_search {
            status.remaining_time = duration.max(status.remaining_time);
        } else {
            self.new_statuses.push(AppliedStatus {
                status_flag,
                remaining_time: duration,
                lifetime: 0,
                destructor: None,
            })
        }
    }

    pub fn set_remaining_status_time(&mut self, status_flag: HitFlags, duration: FrameTime) {
        if duration <= 0 {
            self.remove_status(status_flag);
            return;
        }

        // find existing status
        let mut status_iter = self.statuses.iter_mut();
        let status_search = status_iter.find(|status| status.status_flag == status_flag);

        if let Some(status) = status_search {
            status.remaining_time = duration;
        } else {
            // apply as a new status
            self.apply_status(status_flag, duration);
        }
    }

    pub fn input_locked_out(&self, registry: &StatusRegistry) -> bool {
        self.dragged || self.remaining_drag_lockout > 0 || self.is_inactionable(registry)
    }

    pub fn is_dragged(&self) -> bool {
        self.dragged
    }

    pub fn end_drag(&mut self) {
        self.dragged = false;

        // adding 1 since it's immediately subtracted
        self.remaining_drag_lockout = DRAG_LOCKOUT + 1;
    }

    pub fn is_inactionable(&self, registry: &StatusRegistry) -> bool {
        registry
            .inactionable_flags()
            .iter()
            .any(|&flag| self.remaining_status_time(flag) > 0)
    }

    pub fn is_immobile(&self, registry: &StatusRegistry) -> bool {
        self.remaining_drag_lockout > 0
            || registry
                .immobilizing_flags()
                .iter()
                .any(|&flag| self.remaining_status_time(flag) > 0)
    }

    pub fn is_shaking(&self) -> bool {
        self.remaining_shake_time > 0
    }

    pub fn decrement_shake_time(&mut self) {
        if self.remaining_shake_time > 0 {
            self.remaining_shake_time -= 1;
        }
    }

    pub fn take_status_sprites(&mut self) -> Vec<(HitFlags, TreeIndex)> {
        std::mem::take(&mut self.status_sprites)
    }

    pub fn update_status_sprites(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        entity: &mut Entity,
    ) {
        self.update_status_sprite(game_io, resources, entity, HitFlag::FREEZE);
        self.update_status_sprite(game_io, resources, entity, HitFlag::BLIND);
        self.update_status_sprite(game_io, resources, entity, HitFlag::CONFUSE);
    }

    fn update_status_sprite(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        entity: &mut Entity,
        status_flag: HitFlags,
    ) {
        let sprite_tree = &mut entity.sprite_tree;

        let existing_index = self.status_sprite_index(status_flag);

        let Some(lifetime) = self.status_lifetime(status_flag) else {
            if let Some(sprite_index) = existing_index {
                sprite_tree.remove(sprite_index);
                self.forget_status_sprite(status_flag);
            }
            return;
        };

        let index = existing_index.unwrap_or_else(|| {
            let mut sprite_node = SpriteNode::new(game_io, SpriteColorMode::Add);
            let texture = resources.statuses_texture.clone();
            sprite_node.set_texture_direct(texture);

            let index = sprite_tree.insert_root_child(sprite_node);
            self.status_sprites.push((status_flag, index));
            index
        });

        let alpha = sprite_tree.root().color().a;

        let sprite_node = &mut sprite_tree[index];
        let animator = &mut *resources.statuses_animator.borrow_mut();
        let state = HitFlag::status_animation_state(status_flag, entity.height);

        if animator.current_state() != Some(state) {
            animator.set_state(state);
            animator.set_loop_mode(AnimatorLoopMode::Loop);
        }

        animator.sync_time(lifetime);
        sprite_node.apply_animation(animator);
        sprite_node.set_offset(HitFlag::status_sprite_position(status_flag, entity.height));
        sprite_node.set_alpha(alpha);
    }

    fn status_sprite_index(&self, status_flag: HitFlags) -> Option<TreeIndex> {
        self.status_sprites
            .iter()
            .find(|(flag, _)| *flag == status_flag)
            .map(|(_, index)| *index)
    }

    fn forget_status_sprite(&mut self, status_flag: HitFlags) {
        if let Some(index) = self
            .status_sprites
            .iter()
            .position(|(flag, _)| *flag == status_flag)
        {
            self.status_sprites.remove(index);
        }
    }

    pub fn merge(&mut self, source: Self) {
        for status in source.new_statuses {
            self.apply_status(status.status_flag, status.remaining_time);
        }

        for status in source.statuses {
            self.apply_status(status.status_flag, status.remaining_time);
        }
    }

    pub fn remaining_status_time(&self, status_flag: HitFlags) -> FrameTime {
        self.statuses
            .iter()
            .find(|status| status.status_flag == status_flag)
            .map(|status| status.remaining_time)
            .unwrap_or_default()
    }

    pub fn status_lifetime(&self, status_flag: HitFlags) -> Option<FrameTime> {
        self.statuses
            .iter()
            .find(|status| status.status_flag == status_flag && status.remaining_time > 0)
            .map(|status| status.lifetime)
    }

    pub fn remove_status(&mut self, status_flag: HitFlags) {
        let status_search = self
            .statuses
            .iter()
            .position(|status| status.status_flag == status_flag);

        if let Some(index) = status_search {
            let mut status = self.statuses.remove(index);

            if let Some(callback) = status.destructor.take() {
                self.ready_destructors.push(callback);
            }
        }

        let new_status_search = self
            .new_statuses
            .iter()
            .position(|status| status.status_flag == status_flag);

        if let Some(index) = new_status_search {
            self.new_statuses.remove(index);
        }
    }

    // should be called after update()
    pub fn take_new_statuses(&mut self) -> Vec<HitFlags> {
        std::mem::take(&mut self.new_statuses)
            .into_iter()
            .map(|status| status.status_flag)
            .collect()
    }

    pub fn take_ready_destructors(&mut self) -> Vec<BattleCallback> {
        std::mem::take(&mut self.ready_destructors)
    }

    fn apply_new_statuses(&mut self, registry: &StatusRegistry) {
        let mut already_existing = Vec::new();

        for (i, status) in self.new_statuses.iter().enumerate() {
            let status_flag = status.status_flag;

            let status_search = self
                .statuses
                .iter_mut()
                .find(|status| status.status_flag == status_flag);

            if let Some(prev_status) = status_search {
                if prev_status.remaining_time > 0 {
                    already_existing.push(i);
                }
                prev_status.remaining_time = status.remaining_time.max(prev_status.remaining_time);
            } else {
                self.statuses.push(status.clone());
            }
        }

        already_existing.sort();

        for i in already_existing.into_iter().rev() {
            self.new_statuses.remove(i);
        }

        self.detect_cancelled_statuses(registry);
    }

    fn detect_cancelled_statuses(&mut self, registry: &StatusRegistry) {
        let mut cancelled_statuses = Vec::new();

        for blocker in registry.blockers() {
            if self.remaining_status_time(blocker.blocking_flag) > 0 {
                cancelled_statuses.push(blocker.blocked_flag);
            }
        }

        for status_flag in cancelled_statuses {
            self.remove_status(status_flag);
        }
    }

    pub fn update(&mut self, registry: &StatusRegistry, inputs: &[PlayerInput]) {
        // detect mashing
        let mashed = if let Some(index) = self.input_index {
            let player_input = &inputs[index];

            Input::BATTLE
                .iter()
                .any(|input| player_input.was_just_pressed(*input))
        } else {
            false
        };

        // update remaining time
        for status in &mut self.statuses {
            if mashed && MASHABLE_STATUSES.contains(&status.status_flag) {
                status.remaining_time -= 1;
            }

            status.remaining_time -= 1;
            status.lifetime += 1;

            if status.remaining_time < 0 {
                status.remaining_time = 0;
                status.lifetime = 0;

                if let Some(callback) = status.destructor.take() {
                    self.ready_destructors.push(callback);
                }
            }
        }

        self.apply_new_statuses(registry);

        if self.remaining_drag_lockout > 0 {
            self.remaining_drag_lockout -= 1;
        }
    }
}
