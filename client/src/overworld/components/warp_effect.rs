use super::{Animator, Excluded};
use crate::overworld::{OverworldArea, OverworldEvent};
use crate::render::FrameTime;
use crate::resources::{AssetManager, Globals, ResourcePaths};
use framework::prelude::{GameIO, Vec3};
use packets::structures::Direction;

#[derive(Debug, Clone, Copy)]
pub enum WarpType {
    In {
        position: Vec3,
        direction: Direction,
    },
    Out,
    /// Same as WarpType::Out. On completion, it transforms into WarpType::In
    Full {
        position: Vec3,
        direction: Direction,
    },
}

/// Attaches to the actor
#[derive(Default)]
pub struct WarpController {
    pub warped_out: bool,
    pub walk_timer: Option<FrameTime>,
    pub warp_entity: Option<hecs::Entity>,
}

/// Attaches to the warp animation entity
pub struct WarpEffect {
    pub actor_entity: hecs::Entity,
    pub warp_type: WarpType,
    pub last_frame: Option<usize>,
    pub callback: Option<Box<dyn FnOnce(&mut GameIO, &mut OverworldArea) + Send + Sync>>,
}

impl WarpEffect {
    // returns true if we should proceed with warping
    fn simplify_warp_type(
        area: &mut OverworldArea,
        target_entity: hecs::Entity,
        warp_type: &mut WarpType,
        effect_position: &mut Vec3,
    ) -> bool {
        let entities = &mut area.entities;
        let warp_controller = entities
            .query_one_mut::<&WarpController>(target_entity)
            .unwrap();

        if !warp_controller.warped_out || warp_controller.warp_entity.is_some() {
            return true;
        };

        // already warped out

        match *warp_type {
            WarpType::In { .. } => {
                // no need to update
                true
            }
            WarpType::Out => false,
            WarpType::Full {
                position,
                direction,
            } => {
                // convert WarpType::Full to just WarpType::In
                *effect_position = position;
                *warp_type = WarpType::In {
                    position,
                    direction,
                };
                true
            }
        }
    }

    pub fn spawn(
        game_io: &GameIO,
        area: &mut OverworldArea,
        target_entity: hecs::Entity,
        mut position: Vec3,
        callback: Box<dyn FnOnce(&mut GameIO, &mut OverworldArea) + Send + Sync>,
        mut warp_type: WarpType,
    ) {
        // simplify the warp type
        if !Self::simplify_warp_type(area, target_entity, &mut warp_type, &mut position) {
            // warp type can be skipped, but we still need to use the callback
            // we'll pass it as an event as we don't have mutable access to game_io
            let _ = area.event_sender.send(OverworldEvent::Callback(callback));
            return;
        }

        let globals = Globals::from_resources(game_io);

        // play sfx
        let screen_position = area.map.world_3d_to_screen(position);

        if area.visible && area.world_camera.bounds().contains(screen_position) {
            globals.audio.play_sound(&globals.sfx.warp);
        }

        // create warp effect entity
        let assets = &globals.assets;
        let texture = assets.texture(game_io, ResourcePaths::OVERWORLD_WARP);
        let mut animator = Animator::load_new(assets, ResourcePaths::OVERWORLD_WARP_ANIMATION);

        match warp_type {
            WarpType::In { .. } => {
                animator.set_state("IN");
                Excluded::increment(&mut area.entities, target_entity);
            }
            WarpType::Out | WarpType::Full { .. } => {
                animator.set_state("OUT");
            }
        }

        let warp_entity = area.spawn_artifact(game_io, texture, animator, position);

        let entities = &mut area.entities;
        entities
            .insert_one(
                warp_entity,
                WarpEffect {
                    warp_type,
                    actor_entity: target_entity,
                    last_frame: None,
                    callback: Some(callback),
                },
            )
            .unwrap();

        // update warp controller
        let warp_controller = entities
            .query_one_mut::<&mut WarpController>(target_entity)
            .unwrap();

        let already_warped_out = warp_controller.warped_out;

        warp_controller.warp_entity = Some(warp_entity);
        warp_controller.warped_out = true;

        if already_warped_out {
            // already warped out
            Excluded::decrement(entities, target_entity);
        }
    }

    pub fn warp_out(
        game_io: &GameIO,
        area: &mut OverworldArea,
        target_entity: hecs::Entity,
        callback: impl FnOnce(&mut GameIO, &mut OverworldArea) + Send + Sync + 'static,
    ) {
        let entities = &mut area.entities;
        let position = *entities.query_one_mut::<&Vec3>(target_entity).unwrap();

        Self::spawn(
            game_io,
            area,
            target_entity,
            position,
            Box::new(callback),
            WarpType::Out,
        )
    }

    pub fn warp_in(
        game_io: &GameIO,
        area: &mut OverworldArea,
        target_entity: hecs::Entity,
        position: Vec3,
        direction: Direction,
        callback: impl FnOnce(&mut GameIO, &mut OverworldArea) + Send + Sync + 'static,
    ) {
        Self::spawn(
            game_io,
            area,
            target_entity,
            position,
            Box::new(callback),
            WarpType::In {
                position,
                direction,
            },
        )
    }

    pub fn warp_full(
        game_io: &GameIO,
        area: &mut OverworldArea,
        target_entity: hecs::Entity,
        position: Vec3,
        direction: Direction,
        callback: impl FnOnce(&mut GameIO, &mut OverworldArea) + Send + Sync + 'static,
    ) {
        let entities = &mut area.entities;
        let out_position = *entities.query_one_mut::<&Vec3>(target_entity).unwrap();

        Self::spawn(
            game_io,
            area,
            target_entity,
            out_position,
            Box::new(callback),
            WarpType::Full {
                position,
                direction,
            },
        )
    }
}
