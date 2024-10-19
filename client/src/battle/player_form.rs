use super::{
    Artifact, BattleCallback, BattleSimulation, Entity, Living, Player, PlayerOverridables,
};
use crate::bindable::EntityId;
use crate::resources::Globals;
use framework::prelude::{Texture, Vec2};
use std::sync::Arc;

#[derive(Default, Clone)]
pub struct PlayerForm {
    pub activated: bool,
    pub deactivated: bool,
    pub deactivate_on_weakness: bool,
    pub mug_texture: Option<Arc<Texture>>,
    pub description: Option<Arc<str>>,
    pub activate_callback: Option<BattleCallback>,
    pub deactivate_callback: Option<BattleCallback>,
    pub select_callback: Option<BattleCallback>,
    pub deselect_callback: Option<BattleCallback>,
    pub update_callback: Option<BattleCallback>,
    pub overridables: PlayerOverridables,
}

impl PlayerForm {
    pub fn deactivate(simulation: &mut BattleSimulation, id: EntityId, form_index: usize) {
        simulation.time_freeze_tracker.queue_animation(
            30,
            BattleCallback::new(move |game_io, _, simulation, _| {
                let entities = &mut simulation.entities;

                let Ok((entity, living, player)) =
                    entities.query_one_mut::<(&Entity, &Living, &mut Player)>(id.into())
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
                form.deactivated = true;

                if let Some(callback) = form.deactivate_callback.clone() {
                    simulation.pending_callbacks.push(callback);
                }

                // spawn shine fx
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
