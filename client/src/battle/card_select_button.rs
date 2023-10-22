use super::{BattleCallback, BattleSimulation, Player};
use crate::bindable::EntityId;
use crate::resources::{AssetManager, Globals, ResourcePaths};
use crate::structures::GenerationalIndex;
use framework::prelude::{GameIO, Sprite, Texture};
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
    pub preview_texture: Arc<Texture>,
    pub sprite: Sprite,
    pub animator_index: GenerationalIndex,
    pub activate_callback: Option<BattleCallback<(), bool>>,
    pub undo_callback: Option<BattleCallback>,
}

impl CardSelectButton {
    pub fn new(game_io: &GameIO, animator_index: GenerationalIndex, slot_width: usize) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;
        let blank_texture = assets.texture(game_io, ResourcePaths::BLANK);

        Self {
            slot_width,
            preview_texture: blank_texture.clone(),
            sprite: Sprite::new(game_io, blank_texture),
            animator_index,
            activate_callback: None,
            undo_callback: None,
        }
    }

    pub fn resolve_button_option_mut(
        simulation: &mut BattleSimulation,
        button_path: CardSelectButtonPath,
    ) -> Option<&mut Option<Box<Self>>> {
        let entities = &mut simulation.entities;
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
}
