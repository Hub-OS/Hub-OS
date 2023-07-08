use super::Entity;
use crate::battle::{BattleCallback, BattleSimulation};
use crate::bindable::EntityId;
use crate::resources::{Globals, ResourcePaths};
use framework::prelude::GameIO;

#[derive(Default, Clone)]
pub struct Artifact;

impl Artifact {
    pub fn create(game_io: &GameIO, simulation: &mut BattleSimulation) -> EntityId {
        let id = Entity::create(game_io, simulation);

        simulation
            .entities
            .insert(id.into(), (Artifact::default(),))
            .unwrap();

        let entity = simulation
            .entities
            .query_one_mut::<&mut Entity>(id.into())
            .unwrap();

        entity.ignore_hole_tiles = true;
        entity.ignore_negative_tile_effects = true;
        entity.can_move_to_callback = BattleCallback::stub(true);

        id
    }

    fn create_animated_artifact(
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        texture_path: &str,
        animation_path: &str,
    ) -> EntityId {
        let id = Self::create(game_io, simulation);

        let entity = simulation
            .entities
            .query_one_mut::<&mut Entity>(id.into())
            .unwrap();

        // load texture
        let texture_string = ResourcePaths::absolute(texture_path);
        let sprite_root = entity.sprite_tree.root_mut();
        sprite_root.set_texture(game_io, texture_string);

        // load animation
        let animator = &mut simulation.animators[entity.animator_index];
        let _ = animator.load(game_io, animation_path);
        let _ = animator.set_state("DEFAULT");

        // delete when the animation completes
        animator.on_complete(BattleCallback::new(move |game_io, simulation, vms, _| {
            simulation.mark_entity_for_erasure(game_io, vms, id);
        }));

        id
    }

    pub fn create_explosion(game_io: &GameIO, simulation: &mut BattleSimulation) -> EntityId {
        let id = Self::create_animated_artifact(
            game_io,
            simulation,
            ResourcePaths::BATTLE_EXPLOSION,
            ResourcePaths::BATTLE_EXPLOSION_ANIMATION,
        );

        let entity = simulation
            .entities
            .query_one_mut::<&mut Entity>(id.into())
            .unwrap();

        entity.spawn_callback = BattleCallback::new(|game_io, simulation, _, _| {
            simulation.play_sound(game_io, &game_io.resource::<Globals>().unwrap().sfx.explode);
        });

        id
    }

    pub fn create_alert(game_io: &GameIO, simulation: &mut BattleSimulation) -> EntityId {
        Self::create_animated_artifact(
            game_io,
            simulation,
            ResourcePaths::BATTLE_ALERT,
            ResourcePaths::BATTLE_ALERT_ANIMATION,
        )
    }

    pub fn create_poof(game_io: &GameIO, simulation: &mut BattleSimulation) -> EntityId {
        Self::create_animated_artifact(
            game_io,
            simulation,
            ResourcePaths::BATTLE_POOF,
            ResourcePaths::BATTLE_POOF_ANIMATION,
        )
    }

    pub fn create_transformation_shine(
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
    ) -> EntityId {
        Self::create_animated_artifact(
            game_io,
            simulation,
            ResourcePaths::BATTLE_TRANSFORM_SHINE,
            ResourcePaths::BATTLE_TRANSFORM_SHINE_ANIMATION,
        )
    }

    pub fn create_splash(game_io: &GameIO, simulation: &mut BattleSimulation) -> EntityId {
        Self::create_animated_artifact(
            game_io,
            simulation,
            ResourcePaths::BATTLE_SPLASH,
            ResourcePaths::BATTLE_SPLASH_ANIMATION,
        )
    }
}
