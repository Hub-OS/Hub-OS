use super::{
    BattleAnimator, BattleCallback, BattleSimulation, Player, PlayerOverridables,
    SharedBattleResources,
};
use crate::battle::PlayerHand;
use crate::bindable::{EntityId, SpriteColorMode};
use crate::render::SpriteNode;
use crate::structures::{GenerationalIndex, SlotMap, Tree, TreeIndex};
use crate::{CARD_SELECT_CARD_COLS, CARD_SELECT_ROWS};
use framework::prelude::{GameIO, Vec2};
use std::sync::Arc;

#[derive(Clone, Copy)]
pub struct CardSelectButtonPath {
    pub entity_id: EntityId,
    pub form_index: Option<usize>,
    pub augment_index: Option<GenerationalIndex>,
    pub card_button_slot: usize,
}

impl CardSelectButtonPath {
    fn resolve_mut_overridables<'a>(
        &self,
        entities: &'a mut hecs::World,
    ) -> Option<&'a mut PlayerOverridables> {
        let player = entities
            .query_one_mut::<&mut Player>(self.entity_id.into())
            .ok()?;

        let overridables = if let Some(index) = self.form_index {
            let form = player.forms.get_mut(index)?;
            &mut form.overridables
        } else if let Some(index) = self.augment_index {
            let augments = &mut player.augments;
            let augment = augments.get_mut(index)?;
            &mut augment.overridables
        } else {
            &mut player.overridables
        };

        Some(overridables)
    }
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
    pub const SPECIAL_SLOT: usize = 0;

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
    ) -> Option<&mut Option<Self>> {
        let overridables = button_path.resolve_mut_overridables(entities)?;

        let i = button_path.card_button_slot;

        if i >= overridables.buttons.len() {
            if i > CARD_SELECT_CARD_COLS * CARD_SELECT_ROWS {
                return None;
            }

            overridables.buttons.resize(i + 1, None);
        }

        overridables.buttons.get_mut(i)
    }

    fn iter_card_button_slots_impl<'a, T: std::borrow::Borrow<CardSelectButton>>(
        iter: impl Iterator<Item = (usize, T)> + 'a,
    ) -> impl Iterator<Item = (i32, i32, usize, T)> + 'a {
        const COLS: i32 = CARD_SELECT_CARD_COLS as i32;
        const ROWS: i32 = CARD_SELECT_ROWS as i32;

        let mut col = COLS;
        let mut row = ROWS - 1;

        iter.map_while(move |(i, button)| {
            let slot_width = COLS.min(button.borrow().slot_width as i32);

            col -= slot_width;

            if col < 0 {
                col = COLS - slot_width;
                row -= 1;

                if row < 0 {
                    // there's no more room for buttons
                    return None;
                }
            }

            Some((col, row, i, button))
        })
    }

    pub fn iter_card_button_slots(
        card_buttons: &[Option<Self>],
    ) -> impl Iterator<Item = (i32, i32, usize, &Self)> + '_ {
        Self::iter_card_button_slots_impl(
            card_buttons
                .iter()
                .enumerate()
                .flat_map(|(i, b)| Some((i + 1, b.as_ref()?))),
        )
    }

    pub fn iter_card_button_slots_mut(
        card_buttons: &mut [Option<Self>],
    ) -> impl Iterator<Item = (i32, i32, usize, &mut Self)> + '_ {
        Self::iter_card_button_slots_impl(
            card_buttons
                .iter_mut()
                .enumerate()
                .flat_map(|(i, b)| Some((i + 1, b.as_mut()?))),
        )
    }

    pub fn space_used_by_card_buttons(buttons: &[Option<Self>]) -> usize {
        const COLS: i32 = CARD_SELECT_CARD_COLS as i32;
        const ROWS: i32 = CARD_SELECT_ROWS as i32;

        let Some((col, row, _, _)) = Self::iter_card_button_slots(buttons).last() else {
            return 0;
        };

        let space_used = (ROWS - row - 1) * COLS + (COLS - col);
        space_used as usize
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
        let entities = &mut simulation.entities;
        for (_, (player, hand)) in entities.query_mut::<(&mut Player, &mut PlayerHand)>() {
            if !hand.staged_items.take_updated() {
                continue;
            }

            if let Some(buttons) = PlayerOverridables::card_button_slots_mut_for(player) {
                for button in buttons.iter_mut().flatten() {
                    if let Some(callback) = button.selection_change_callback.clone() {
                        simulation.pending_callbacks.push(callback);
                    }
                }
            }

            if let Some(button) = PlayerOverridables::special_button_mut_for(player)
                && let Some(callback) = button.selection_change_callback.clone()
            {
                simulation.pending_callbacks.push(callback);
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
