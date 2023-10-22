use super::{BattleAnimator, BattleCallback, Player};
use crate::bindable::EntityId;
use crate::resources::{AssetManager, Globals, ResourcePaths};
use crate::structures::{GenerationalIndex, SlotMap};
use framework::prelude::{GameIO, Sprite, Vec2};

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
    pub sprite: Sprite,
    pub animator_index: GenerationalIndex,
    pub preview_sprite: Sprite,
    pub preview_animator_index: GenerationalIndex,
    pub activate_callback: Option<BattleCallback<(), bool>>,
    pub undo_callback: Option<BattleCallback>,
}

impl CardSelectButton {
    pub fn new(
        game_io: &GameIO,
        animators: &mut SlotMap<BattleAnimator>,
        slot_width: usize,
    ) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let blank_texture = assets.texture(game_io, ResourcePaths::BLANK);
        let blank_sprite = Sprite::new(game_io, blank_texture);

        let animator_index = animators.insert(BattleAnimator::new());
        let preview_animator_index = animators.insert(BattleAnimator::new());

        Self {
            slot_width,
            sprite: blank_sprite.clone(),
            animator_index,
            preview_sprite: blank_sprite,
            preview_animator_index,
            activate_callback: None,
            undo_callback: None,
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
        animators: &mut SlotMap<BattleAnimator>,
        pending_callbacks: &mut Vec<BattleCallback>,
        position: Vec2,
    ) {
        let animator = &mut animators[self.animator_index];
        pending_callbacks.extend(animator.update());
        animator.animator().apply(&mut self.sprite);

        self.sprite.set_position(position);
    }

    pub fn animate_preview_sprite(
        &mut self,
        animators: &mut SlotMap<BattleAnimator>,
        pending_callbacks: &mut Vec<BattleCallback>,
        position: Vec2,
    ) {
        let preview_animator = &mut animators[self.preview_animator_index];
        let callbacks = preview_animator.update();
        preview_animator.animator().apply(&mut self.preview_sprite);
        pending_callbacks.extend(callbacks);

        self.preview_sprite.set_position(position);
    }
}
