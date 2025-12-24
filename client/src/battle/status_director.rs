use super::{BattleCallback, Entity, SharedBattleResources, StatusRegistry};
use crate::bindable::{Drag, HitFlag, HitFlags, SpriteColorMode};
use crate::render::{AnimatorLoopMode, FrameTime, SpriteNode, TreeIndex};
use crate::resources::DRAG_LOCKOUT;
use crate::structures::{Tree, VecMap};
use framework::prelude::GameIO;
use packets::structures::Direction;

#[derive(Clone)]
struct AppliedStatus {
    status_flag: HitFlags,
    remaining_time: FrameTime,
    lifetime: FrameTime,
    destructor: Option<BattleCallback>,
    reapplied: bool,
}

#[derive(Clone, Default)]
pub struct StatusDirector {
    immunity: HitFlags,
    statuses: Vec<AppliedStatus>,
    new_statuses: Vec<AppliedStatus>,
    ready_destructors: Vec<BattleCallback>,
    status_sprites: Vec<(HitFlags, TreeIndex)>,
    drag: Option<Drag>,
    remaining_drag_lockout: u8,
    remaining_shake: u8,
}

impl StatusDirector {
    pub fn clear_statuses(&mut self, status_flags: HitFlags) {
        self.ready_destructors.extend(
            self.statuses
                .iter_mut()
                .filter(|s| status_flags & s.status_flag != 0)
                .flat_map(|status| status.destructor.take()),
        );

        self.statuses.retain(|s| status_flags & s.status_flag == 0);
        self.new_statuses
            .retain(|s| status_flags & s.status_flag == 0);
        self.drag = None;
        self.remaining_drag_lockout = 0;
    }

    pub fn applied_and_pending(&self, status_flags: HitFlags) -> Vec<(HitFlags, FrameTime)> {
        self.statuses
            .iter()
            .filter(|status| status_flags & status.status_flag != 0 && status.remaining_time > 0)
            .map(|status| (status.status_flag, status.remaining_time))
            .chain(
                self.new_statuses
                    .iter()
                    .map(|status| (status.status_flag, status.remaining_time)),
            )
            .collect()
    }

    pub fn clear_immunity(&mut self) {
        self.immunity = HitFlag::NONE;
    }

    pub fn add_immunity(&mut self, hit_flags: HitFlags) {
        self.immunity |= hit_flags;
    }

    pub fn immunities(&self) -> HitFlags {
        self.immunity
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

    pub fn apply_hit_flags(
        &mut self,
        status_registry: &StatusRegistry,
        mut hit_flags: HitFlags,
        durations: &VecMap<HitFlags, FrameTime>,
    ) {
        hit_flags &= !self.immunity;

        // handle blocked_by / blocks_flags
        for registered_status in status_registry.registered_list() {
            if hit_flags & registered_status.flag == HitFlag::NONE {
                continue;
            }

            if hit_flags & status_registry.overrides_for(registered_status.flag) != HitFlag::NONE {
                hit_flags &= !registered_status.flag;
            }
        }

        for hit_flag in HitFlag::BAKED {
            if hit_flags & hit_flag == HitFlag::NONE {
                continue;
            }

            self.apply_status(hit_flag, 1);
        }

        for registered_status in status_registry.registered_list() {
            if hit_flags & registered_status.flag == HitFlag::NONE {
                continue;
            }

            let duration = durations
                .get(&registered_status.flag)
                .cloned()
                .unwrap_or_else(|| registered_status.duration_for(1));

            self.apply_status(registered_status.flag, duration);
        }
    }

    pub fn apply_status(&mut self, status_flag: HitFlags, duration: FrameTime) {
        self.apply_status_internal(AppliedStatus {
            status_flag,
            remaining_time: duration,
            lifetime: 0,
            destructor: None,
            reapplied: false,
        });
    }

    pub fn reapply_status(&mut self, status_flag: HitFlags, duration: FrameTime) {
        self.apply_status_internal(AppliedStatus {
            status_flag,
            remaining_time: duration,
            lifetime: 0,
            destructor: None,
            reapplied: true,
        });
    }

    fn apply_status_internal(&mut self, new_status: AppliedStatus) {
        if new_status.status_flag & self.immunity != 0 {
            return;
        }

        self.new_statuses
            .retain(|status| status.status_flag != new_status.status_flag);

        self.new_statuses.push(new_status);
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
        self.is_dragged() || self.remaining_drag_lockout > 0 || self.is_inactionable(registry)
    }

    pub fn is_dragged(&self) -> bool {
        self.drag.is_some()
    }

    pub fn set_drag(&mut self, drag: Drag) {
        self.drag = Some(drag);
    }

    pub fn take_next_drag_movement(&mut self) -> Direction {
        let Some(drag) = &mut self.drag else {
            return Direction::None;
        };

        if drag.distance == 0 {
            self.end_drag();
            return Direction::None;
        }

        drag.distance -= 1;
        drag.direction
    }

    pub fn take_drag_for_backup(&mut self) -> Option<Drag> {
        self.drag.take()
    }

    pub fn end_drag(&mut self) {
        self.drag = None;

        // adding 1 since it's immediately subtracted
        self.remaining_drag_lockout = DRAG_LOCKOUT + 1;
    }

    pub fn remaining_drag_lockout(&self) -> u8 {
        self.remaining_drag_lockout
    }

    pub fn set_remaining_drag_lockout(&mut self, time: u8) {
        self.remaining_drag_lockout = time;
    }

    pub fn remaining_shake(&self) -> u8 {
        self.remaining_shake
    }

    pub fn start_shake(&mut self) {
        self.remaining_shake = 30;
    }

    pub fn set_remaining_shake(&mut self, remaining: u8) {
        self.remaining_shake = remaining;
    }

    pub fn is_inactionable(&self, registry: &StatusRegistry) -> bool {
        self.remaining_drag_lockout > 0
            || self.statuses.iter().any(|status| {
                registry.inactionable_flags() & status.status_flag != 0 && status.remaining_time > 0
            })
    }

    pub fn is_immobile(&self, registry: &StatusRegistry) -> bool {
        self.drag.is_none()
            && self.statuses.iter().any(|status| {
                registry.immobilizing_flags() & status.status_flag != 0 && status.remaining_time > 0
            })
    }

    pub fn take_status_sprites(&mut self, status_flags: HitFlags) -> Vec<(HitFlags, TreeIndex)> {
        let mut old_sprites = std::mem::take(&mut self.status_sprites);

        old_sprites.retain(|&(flag, index)| {
            let retain = status_flags & flag != 0;

            if !retain {
                self.status_sprites.push((flag, index));
            }

            retain
        });

        old_sprites
    }

    pub fn update_status_sprites(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        entity: &mut Entity,
        sprite_tree: &mut Tree<SpriteNode>,
    ) {
        self.update_status_sprite(game_io, resources, entity, sprite_tree, HitFlag::BLIND);
        self.update_status_sprite(game_io, resources, entity, sprite_tree, HitFlag::CONFUSE);
    }

    fn update_status_sprite(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        entity: &mut Entity,
        sprite_tree: &mut Tree<SpriteNode>,
        status_flag: HitFlags,
    ) {
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
        let state = HitFlag::status_animation_state(status_flag);

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

    pub fn remove_statuses(&mut self, mut status_flags: HitFlags) {
        let mut i = 0;

        loop {
            if status_flags == 0 {
                break;
            }

            if status_flags & 1 == 1 {
                self.remove_status(1 << i);
            }

            status_flags >>= 1;

            i += 1;
        }
    }

    fn remove_status(&mut self, status_flag: HitFlags) {
        let status_search = self
            .statuses
            .iter_mut()
            .find(|status| status.status_flag == status_flag);

        if let Some(status) = status_search {
            status.remaining_time = 0;
            status.lifetime = 0;
        }

        let new_status_search = self
            .new_statuses
            .iter()
            .position(|status| status.status_flag == status_flag);

        if let Some(index) = new_status_search {
            self.new_statuses.remove(index);
        }

        if status_flag == HitFlag::DRAG {
            self.drag = None;
            self.remaining_drag_lockout = 0;
        }
    }

    // should be called after update()
    pub fn take_new_statuses(&mut self) -> Vec<(HitFlags, bool)> {
        self.new_statuses
            .drain(..)
            .map(|status| (status.status_flag, status.reapplied))
            .collect()
    }

    pub fn take_ready_destructors(&mut self) -> Vec<BattleCallback> {
        // append some final statuses
        self.statuses.retain_mut(|status| {
            if status.remaining_time > 0 {
                return true;
            }

            if let Some(callback) = status.destructor.take() {
                self.ready_destructors.push(callback);
            }

            false
        });

        std::mem::take(&mut self.ready_destructors)
    }

    pub fn apply_new_statuses(&mut self, registry: &StatusRegistry) {
        self.resolve_conflicts(registry);

        self.new_statuses.retain(|status| {
            let status_flag = status.status_flag;

            let status_search = self
                .statuses
                .iter_mut()
                .find(|status| status.status_flag == status_flag);

            let Some(prev_status) = status_search else {
                // new status
                self.statuses.push(status.clone());
                return true;
            };

            prev_status.remaining_time = status.remaining_time.max(prev_status.remaining_time);
            prev_status.reapplied = status.reapplied;

            false
        });
    }

    fn resolve_conflicts(&mut self, registry: &StatusRegistry) {
        let mut cancelled_statuses = Vec::new();

        // reverse to preserve the newest statuses
        self.new_statuses.reverse();
        self.new_statuses.retain(|status| {
            let flag = status.status_flag;

            for &cancelled_flag in registry.mutual_exclusions_for(flag) {
                cancelled_statuses.push(cancelled_flag);
            }

            !cancelled_statuses.contains(&flag)
        });

        // reverse again to preserve application order
        self.new_statuses.reverse();

        for status_flag in cancelled_statuses {
            // protect the status if we're going to add it again
            let protected = self
                .new_statuses
                .iter()
                .any(|status| status.status_flag == status_flag);

            if !protected {
                self.remove_status(status_flag);
            }
        }
    }

    pub fn update(&mut self) {
        // update remaining time
        for status in &mut self.statuses {
            status.remaining_time -= 1;
            status.lifetime += 1;
        }

        if self.remaining_drag_lockout > 0 {
            self.remaining_drag_lockout -= 1;
        }
    }
}
