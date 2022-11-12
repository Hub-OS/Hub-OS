use generational_arena::Arena;

use super::{BattleAnimator, BattleCallback};
use crate::bindable::{ActionLockout, CardProperties, EntityID, GenerationalIndex};
use crate::render::{DerivedFrame, FrameTime, SpriteNode, Tree};

#[derive(Clone)]
pub struct CardAction {
    pub active_frames: FrameTime,
    pub deleted: bool,
    pub executed: bool,
    pub used: bool,
    pub entity: EntityID,
    pub state: String,
    pub prev_state: Option<String>,
    pub frame_callbacks: Vec<(usize, BattleCallback)>,
    pub sprite_index: GenerationalIndex,
    pub properties: CardProperties,
    pub derived_frames: Option<Vec<DerivedFrame>>,
    pub steps: Vec<CardActionStep>,
    pub step_index: usize,
    pub attachments: Vec<CardActionAttachment>,
    pub lockout_type: ActionLockout,
    pub time_freeze_blackout_tiles: bool,
    pub can_move_to_callback: Option<BattleCallback<(i32, i32), bool>>,
    pub update_callback: Option<BattleCallback>,
    pub execute_callback: Option<BattleCallback>,
    pub end_callback: Option<BattleCallback>,
    pub animation_end_callback: Option<BattleCallback>,
}

impl CardAction {
    pub fn new(entity: EntityID, state: String, sprite_index: GenerationalIndex) -> Self {
        Self {
            active_frames: 0,
            deleted: false,
            executed: false,
            used: false,
            entity,
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
            time_freeze_blackout_tiles: false,
            can_move_to_callback: None,
            update_callback: None,
            execute_callback: None,
            end_callback: None,
            animation_end_callback: None,
        }
    }

    pub fn is_async(&self) -> bool {
        matches!(self.lockout_type, ActionLockout::Async(_))
    }
}

#[derive(Clone)]
pub struct CardActionAttachment {
    pub point_name: String,
    pub sprite_index: GenerationalIndex,
    pub animator_index: generational_arena::Index,
    pub parent_animator_index: generational_arena::Index,
}

impl CardActionAttachment {
    pub fn new(
        point_name: String,
        sprite_index: GenerationalIndex,
        animator_index: generational_arena::Index,
        parent_animator_index: generational_arena::Index,
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
        animators: &mut Arena<BattleAnimator>,
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
pub struct CardActionStep {
    pub completed: bool,
    pub callback: BattleCallback,
}
