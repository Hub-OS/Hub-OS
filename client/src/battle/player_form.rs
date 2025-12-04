use super::{
    Artifact, BattleCallback, BattleSimulation, Entity, Living, Player, PlayerOverridables,
};
use crate::resources::Globals;
use crate::{battle::PlayerHand, bindable::EntityId};
use framework::prelude::{Texture, Vec2};
use std::sync::Arc;

#[derive(Clone)]
pub struct PlayerForm {
    pub hidden: bool,
    pub activated: bool,
    pub deactivated: bool,
    pub close_on_select: bool,
    pub transition_on_select: bool,
    pub mug_texture: Option<Arc<Texture>>,
    pub description: Option<Arc<str>>,
    pub activate_callback: Option<BattleCallback>,
    pub deactivate_callback: Option<BattleCallback>,
    pub select_callback: Option<BattleCallback>,
    pub deselect_callback: Option<BattleCallback>,
    pub update_callback: Option<BattleCallback>,
    pub overridables: PlayerOverridables,
}

impl Default for PlayerForm {
    fn default() -> Self {
        Self {
            hidden: false,
            activated: false,
            deactivated: false,
            close_on_select: true,
            transition_on_select: true,
            mug_texture: Default::default(),
            description: Default::default(),
            activate_callback: Default::default(),
            deactivate_callback: Default::default(),
            select_callback: Default::default(),
            deselect_callback: Default::default(),
            update_callback: Default::default(),
            overridables: Default::default(),
        }
    }
}

impl PlayerForm {
    pub fn deactivate(simulation: &mut BattleSimulation, id: EntityId, form_index: usize) {
        let entities = &mut simulation.entities;

        let Ok((player, hand)) =
            entities.query_one_mut::<(&mut Player, &mut PlayerHand)>(id.into())
        else {
            return;
        };

        let form = &mut player.forms[form_index];
        form.deactivated = true;
        form.activated = true;

        if hand.staged_items.stored_form_index() == Some(form_index) {
            if let Some(callback) = &form.deselect_callback {
                simulation.pending_callbacks.push(callback.clone());
            }

            if let Some((_, Some(undo_callback))) = hand.staged_items.drop_form_selection() {
                simulation.pending_callbacks.push(undo_callback);
            }
        }

        if player.active_form != Some(form_index) {
            // this form isn't active, we don't need to do anything else
            return;
        }

        simulation.time_freeze_tracker.queue_animation(
            30,
            BattleCallback::new(move |game_io, _, simulation, _| {
                let entities = &mut simulation.entities;

                let Ok((entity, living, player)) =
                    entities.query_one_mut::<(&Entity, &mut Living, &mut Player)>(id.into())
                else {
                    return;
                };

                if living.health <= 0 || entity.deleted {
                    // skip decross if we're already deleted
                    return;
                }

                if player.active_form != Some(form_index) {
                    // this is a different form
                    // must have switched in cust with some frame perfect shenanigans
                    return;
                }

                player.active_form = None;

                let form = &mut player.forms[form_index];

                if let Some(callback) = form.deactivate_callback.clone() {
                    simulation.pending_callbacks.push(callback);
                }

                // resolve shine fx position
                let mut shine_position = entity.full_position();
                shine_position.offset += Vec2::new(0.0, -entity.height * 0.5);

                // play revert sfx
                let sfx = &game_io.resource::<Globals>().unwrap().sfx;
                simulation.play_sound(game_io, &sfx.form_deactivate);

                // actual shine creation as indicated above
                let shine_id = Artifact::create_transformation_shine(game_io, simulation);
                let shine_entity = simulation
                    .entities
                    .query_one_mut::<&mut Entity>(shine_id.into())
                    .unwrap();

                // shine position, set to spawn
                shine_entity.copy_full_position(shine_position);
                shine_entity.pending_spawn = true;
            }),
        );
    }
}
