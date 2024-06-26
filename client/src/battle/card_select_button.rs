use super::{
    BattleAnimator, BattleCallback, BattleSimulation, Player, PlayerOverridables,
    SharedBattleResources,
};
use crate::bindable::{EntityId, SpriteColorMode};
use crate::render::SpriteNode;
use crate::structures::{GenerationalIndex, SlotMap, Tree, TreeIndex};
use framework::prelude::{GameIO, Vec2};
use std::sync::Arc;

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
    pub description: Option<Arc<str>>,
    pub uses_default_audio: bool,
    pub uses_fixed_card_cursor: bool,
    pub use_callback: Option<BattleCallback<(), bool>>,
    pub selection_change_callback: Option<BattleCallback>,
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
        let mut animator = BattleAnimator::new();
        animator.set_target(sprite_tree_index, TreeIndex::tree_root());

        let mut preview_animator = BattleAnimator::new();
        preview_animator.set_target(preview_sprite_tree_index, TreeIndex::tree_root());

        let animator_index = animators.insert(animator);
        let preview_animator_index = animators.insert(preview_animator);

        Self {
            slot_width,
            sprite_tree_index,
            animator_index,
            preview_sprite_tree_index,
            preview_animator_index,
            description: None,
            uses_default_audio: true,
            uses_fixed_card_cursor: false,
            use_callback: None,
            selection_change_callback: None,
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

    pub fn update_all_from_staged_items(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        for (_, player) in simulation.entities.query_mut::<&mut Player>() {
            if !player.staged_items.take_updated() {
                continue;
            }

            let card_button = PlayerOverridables::flat_map_mut_for(player, |overridables| {
                overridables.card_button.as_mut()
            })
            .next();

            if let Some(button) = card_button {
                if let Some(callback) = button.selection_change_callback.clone() {
                    simulation.pending_callbacks.push(callback);
                }
            }

            let special_button = PlayerOverridables::flat_map_mut_for(player, |overridables| {
                overridables.special_button.as_mut()
            })
            .next();

            if let Some(button) = special_button {
                if let Some(callback) = button.selection_change_callback.clone() {
                    simulation.pending_callbacks.push(callback);
                }
            }
        }

        simulation.call_pending_callbacks(game_io, resources);
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
