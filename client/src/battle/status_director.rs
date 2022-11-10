use crate::bindable::{HitFlag, HitFlags};
use crate::render::FrameTime;
use crate::resources::{
    BATTLE_INPUTS, DEFAULT_INTANGIBILITY_DURATION, DEFAULT_STATUS_DURATION, DRAG_LOCKOUT,
};

use super::PlayerInput;

struct StatusBlocker {
    blocking_flag: HitFlags, // The flag that prevents the other from going through.
    blocked_flag: HitFlags,  // The flag that is being prevented.
}

// statuses that cancel other status effects
const STATUS_BLOCKERS: [StatusBlocker; 4] = [
    StatusBlocker {
        blocking_flag: HitFlag::FREEZE,
        blocked_flag: HitFlag::PARALYZE,
    },
    StatusBlocker {
        blocking_flag: HitFlag::PARALYZE,
        blocked_flag: HitFlag::FREEZE,
    },
    StatusBlocker {
        blocking_flag: HitFlag::BUBBLE,
        blocked_flag: HitFlag::FREEZE,
    },
    StatusBlocker {
        blocking_flag: HitFlag::CONFUSE,
        blocked_flag: HitFlag::FREEZE,
    },
];

const MASHABLE_STATUSES: [HitFlags; 3] = [HitFlag::PARALYZE, HitFlag::FREEZE, HitFlag::BUBBLE];

#[derive(Clone)]
struct AppliedStatus {
    status_flag: HitFlags,
    remaining_time: FrameTime,
}

#[derive(Clone)]
pub struct StatusDirector {
    statuses: Vec<AppliedStatus>,
    new_statuses: Vec<AppliedStatus>,
    input_index: Option<usize>,
    dragged: bool,
    remaining_drag_lockout: FrameTime,
}

impl Default for StatusDirector {
    fn default() -> Self {
        Self {
            statuses: Vec::new(),
            new_statuses: Vec::new(),
            input_index: None,
            dragged: false,
            remaining_drag_lockout: 0,
        }
    }
}

impl StatusDirector {
    pub fn set_input_index(&mut self, input_index: usize) {
        self.input_index = Some(input_index);
    }

    pub fn apply_hit_flags(&mut self, hit_flags: HitFlags) {
        for hit_flag in HitFlag::LIST {
            if hit_flags & hit_flag == HitFlag::NONE {
                continue;
            }

            if hit_flag == HitFlag::DRAG {
                self.dragged = true;
            }

            let duration = if HitFlag::STATUS_LIST.contains(&hit_flag) {
                DEFAULT_STATUS_DURATION
            } else if hit_flag == HitFlag::FLASH {
                DEFAULT_INTANGIBILITY_DURATION
            } else {
                0
            };

            self.apply_status(hit_flag, duration);
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
            })
        }
    }

    pub fn input_locked_out(&self) -> bool {
        self.dragged || self.remaining_drag_lockout > 0 || self.is_inactionable()
    }

    pub fn is_dragged(&self) -> bool {
        self.dragged
    }

    pub fn end_drag(&mut self) {
        self.dragged = false;

        // adding 1 since it's immediately subtracted
        self.remaining_drag_lockout = DRAG_LOCKOUT + 1;
    }

    pub fn is_inactionable(&self) -> bool {
        self.remaining_status_time(HitFlag::PARALYZE) > 0
            || self.remaining_status_time(HitFlag::BUBBLE) > 0
            || self.remaining_status_time(HitFlag::FREEZE) > 0
    }

    pub fn is_immobile(&self) -> bool {
        self.remaining_drag_lockout > 0
            || self.remaining_status_time(HitFlag::PARALYZE) > 0
            || self.remaining_status_time(HitFlag::BUBBLE) > 0
            || self.remaining_status_time(HitFlag::ROOT) > 0
            || self.remaining_status_time(HitFlag::FREEZE) > 0
    }

    pub fn remaining_status_time(&self, status_flag: HitFlags) -> FrameTime {
        self.statuses
            .iter()
            .find(|status| status.status_flag == status_flag)
            .map(|status| status.remaining_time)
            .unwrap_or_default()
    }

    pub fn remove_status(&mut self, status_flag: HitFlags) {
        let status_search = self
            .statuses
            .iter()
            .position(|status| status.status_flag == status_flag);

        if let Some(index) = status_search {
            self.statuses.remove(index);
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

    fn apply_new_statuses(&mut self) {
        let mut already_existing = Vec::new();

        for status in &self.new_statuses {
            let status_flag = status.status_flag;

            let status_search = self
                .statuses
                .iter_mut()
                .enumerate()
                .find(|(_, status)| status.status_flag == status_flag);

            if let Some((i, prev_status)) = status_search {
                if prev_status.remaining_time > 0 {
                    already_existing.push(i)
                }
                prev_status.remaining_time = status.remaining_time.max(prev_status.remaining_time);
            } else {
                self.statuses.push(status.clone());
            }
        }

        for i in already_existing.into_iter().rev() {
            self.new_statuses.remove(i);
        }

        self.detect_cancelled_statuses();
    }

    fn detect_cancelled_statuses(&mut self) {
        let mut cancelled_statuses = Vec::new();

        for blocker in &STATUS_BLOCKERS {
            if self.remaining_status_time(blocker.blocking_flag) > 0 {
                cancelled_statuses.push(blocker.blocked_flag);
            }
        }

        for status_flag in cancelled_statuses {
            self.remove_status(status_flag);
        }
    }

    pub fn update(&mut self, inputs: &[PlayerInput]) {
        // detect mashing
        let mashed = if let Some(index) = self.input_index {
            let player_input = &inputs[index];

            BATTLE_INPUTS
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

            if status.remaining_time < 0 {
                status.remaining_time = 0;
            }
        }

        self.apply_new_statuses();

        if self.remaining_drag_lockout > 0 {
            self.remaining_drag_lockout -= 1;
        }
    }
}
