use super::{BattleAnimator, BattleCallback, Player};
use crate::bindable::{EntityId, SpriteColorMode};
use crate::render::SpriteNode;
use crate::structures::{GenerationalIndex, SlotMap, Tree, TreeIndex};
use framework::prelude::{GameIO, Vec2};

#[derive(Clone, Copy)]
pub struct CardSelectButtonPath {
    pub entity_id: EntityId,
    pub form_index: Option<usize>,
    pub augment_index: Option<GenerationalIndex>,
    pub uses_card_slots: bool,
}

#[derive(Clone)]
pub struct CardSelectButton {
    pub slot_width: usize,
    pub sprite_tree_index: GenerationalIndex,
    pub animator_index: GenerationalIndex,
    pub preview_sprite_tree_index: TreeIndex,
    pub preview_animator_index: GenerationalIndex,
    pub uses_default_audio: bool,
    pub use_callback: Option<BattleCallback<(), bool>>,
}

impl CardSelectButton {
    pub fn new(
        game_io: &GameIO,
        sprite_trees: &mut SlotMap<Tree<SpriteNode>>,
        animators: &mut SlotMap<BattleAnimator>,
        slot_width: usize,
    ) -> Self {
        // sprites
        let default_node = SpriteNode::new(game_io, SpriteColorMode::Add);
        let sprite_tree = Tree::new(default_node.clone());
        let preview_sprite_tree = Tree::new(default_node.clone());

        let sprite_tree_index = sprite_trees.insert(sprite_tree);
        let preview_sprite_tree_index = sprite_trees.insert(preview_sprite_tree);

        // animators
        let animator_index = animators.insert(BattleAnimator::new());
        let preview_animator_index = animators.insert(BattleAnimator::new());

        Self {
            slot_width,
            sprite_tree_index,
            animator_index,
            preview_sprite_tree_index,
            preview_animator_index,
            uses_default_audio: true,
            use_callback: None,
        }
    }

    pub fn resolve_button_option_mut(
        entities: &mut hecs::World,
        button_path: CardSelectButtonPath,
    ) -> Option<&mut Option<Box<Self>>> {
        let player = entities
            .query_one_mut::<&mut Player>(button_path.entity_id.into())
            .ok()?;

        let overridables = if let Some(index) = button_path.form_index {
            let form = player.forms.get_mut(index)?;
            &mut form.overridables
        } else if let Some(index) = button_path.augment_index {
            let augments = &mut player.augments;
            let augment = augments.get_mut(index)?;
            &mut augment.overridables
        } else {
            &mut player.overridables
        };

        let button_option = if button_path.uses_card_slots {
            &mut overridables.card_button
        } else {
            &mut overridables.special_button
        };

        Some(button_option)
    }

    pub fn animate_sprite(
        &mut self,
        sprite_trees: &mut SlotMap<Tree<SpriteNode>>,
        animators: &mut SlotMap<BattleAnimator>,
        pending_callbacks: &mut Vec<BattleCallback>,
        position: Vec2,
    ) {
        if let Some(sprite_tree) = sprite_trees.get_mut(self.sprite_tree_index) {
            let sprite_node = sprite_tree.root_mut();

            let preview_animator = &mut animators[self.animator_index];
            let callbacks = preview_animator.update();
            preview_animator.apply(sprite_node);
            pending_callbacks.extend(callbacks);

            sprite_node.set_offset(position);
        }
    }

    pub fn animate_preview_sprite(
        &mut self,
        sprite_trees: &mut SlotMap<Tree<SpriteNode>>,
        animators: &mut SlotMap<BattleAnimator>,
        pending_callbacks: &mut Vec<BattleCallback>,
        position: Vec2,
    ) {
        if let Some(sprite_tree) = sprite_trees.get_mut(self.preview_sprite_tree_index) {
            let sprite_node = sprite_tree.root_mut();

            let preview_animator = &mut animators[self.preview_animator_index];
            let callbacks = preview_animator.update();
            preview_animator.apply(sprite_node);
            pending_callbacks.extend(callbacks);

            sprite_node.set_offset(position);
        }
    }

    pub fn delete_self(
        self,
        sprite_trees: &mut SlotMap<Tree<SpriteNode>>,
        animators: &mut SlotMap<BattleAnimator>,
    ) {
        sprite_trees.remove(self.sprite_tree_index);
        sprite_trees.remove(self.preview_sprite_tree_index);
        animators.remove(self.animator_index);
        animators.remove(self.preview_animator_index);
    }
}
