use crate::bindable::{Element, HitFlag, HitFlags, HitProperties};
use crate::render::FrameTime;

use super::BattleCallback;

#[derive(Clone)]
pub struct IntangibleRule {
    pub duration: FrameTime,
    pub hit_weaknesses: HitFlags,
    pub element_weaknesses: Vec<Element>,
    pub deactivate_callback: Option<BattleCallback>,
}

impl Default for IntangibleRule {
    fn default() -> Self {
        Self {
            duration: 120,
            hit_weaknesses: HitFlag::PIERCE_INVIS,
            element_weaknesses: Vec::new(),
            deactivate_callback: None,
        }
    }
}

#[derive(Default, Clone)]
pub struct Intangibility {
    pub deactivate_callback: Option<BattleCallback>,
    remaining_duration: FrameTime,
    hit_weaknesses: HitFlags,
    element_weaknesses: Vec<Element>,
    retangible: bool,
    just_deactivated: bool,
}

impl Intangibility {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn enable(&mut self, rule: IntangibleRule) {
        self.remaining_duration = rule.duration;
        self.hit_weaknesses = rule.hit_weaknesses;
        self.element_weaknesses = rule.element_weaknesses;
        self.deactivate_callback = rule.deactivate_callback;
    }

    pub fn is_enabled(&self) -> bool {
        self.remaining_duration > 0
    }

    pub fn update(&mut self) {
        if self.just_deactivated {
            debug_assert!(
                self.deactivate_callback.is_none(),
                "deactivate callback must be used"
            );

            self.just_deactivated = false;
        }

        if self.remaining_duration == 0 {
            return;
        }

        self.remaining_duration -= 1;

        if self.remaining_duration == 0 {
            self.just_deactivated = true;
            self.retangible = false;
        }
    }

    pub fn just_deactivated(&self) -> bool {
        self.just_deactivated
    }

    pub fn disable(&mut self) {
        if self.remaining_duration == 0 {
            return;
        }

        self.just_deactivated = true;
        self.remaining_duration = 0;
        self.retangible = false;
    }

    pub fn is_retangible(&self) -> bool {
        self.retangible
    }

    pub fn try_pierce(&mut self, statuses: &HitProperties) -> bool {
        if !self.pierces(statuses) {
            return false;
        }

        if (statuses.flags & HitFlag::RETAIN_INTANGIBLE) == HitFlag::NONE {
            self.retangible = true;
        }

        true
    }

    fn pierces(&self, statuses: &HitProperties) -> bool {
        if (statuses.flags & self.hit_weaknesses) != HitFlag::NONE {
            // any hit weakness will make this test pass
            return true;
        }

        // test for any elem weakness
        return self
            .element_weaknesses
            .iter()
            .any(|element| *element == statuses.element || *element == statuses.secondary_element);
    }
}