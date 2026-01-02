use super::Entity;
use crate::battle::{
    BattleCallback, BattleSimulation, CanMoveToCallback, Component, SpawnCallback,
};
use crate::bindable::EntityId;
use crate::resources::{Globals, ResourcePaths};
use framework::prelude::GameIO;

#[derive(Default, Clone)]
pub struct Artifact;

impl Artifact {
    pub fn create(game_io: &GameIO, simulation: &mut BattleSimulation) -> EntityId {
        let id = Entity::create(game_io, simulation);

        let components = (
            Artifact::default(),
            CanMoveToCallback(BattleCallback::stub(true)),
        );

        simulation.entities.insert(id.into(), components).unwrap();

        let entity = simulation
            .entities
            .query_one_mut::<&mut Entity>(id.into())
            .unwrap();

        entity.ignore_hole_tiles = true;
        entity.ignore_negative_tile_effects = true;

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
        if let Some(sprite_tree) = simulation.sprite_trees.get_mut(entity.sprite_tree_index) {
            let texture_string = ResourcePaths::game_folder_absolute(texture_path);
            let sprite_root = sprite_tree.root_mut();
            sprite_root.set_texture(game_io, texture_string);
        }

        // load animation
        let animator = &mut simulation.animators[entity.animator_index];
        let _ = animator.load(game_io, animation_path);
        let _ = animator.set_state("DEFAULT");

        // delete when the animation completes
        animator.on_complete(BattleCallback::new(
            move |game_io, resources, simulation, _| {
                Entity::mark_erased(game_io, resources, simulation, id);
            },
        ));

        id
    }

    pub fn create_explosion(game_io: &GameIO, simulation: &mut BattleSimulation) -> EntityId {
        let id = Self::create_animated_artifact(
            game_io,
            simulation,
            ResourcePaths::BATTLE_EXPLOSION,
            ResourcePaths::BATTLE_EXPLOSION_ANIMATION,
        );

        let spawn_callback = BattleCallback::new(|game_io, _, simulation, _| {
            simulation.play_sound(game_io, &Globals::from_resources(game_io).sfx.explode);
        });

        let _ = simulation
            .entities
            .insert_one(id.into(), SpawnCallback(spawn_callback));

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

    pub fn create_trap_alert(game_io: &GameIO, simulation: &mut BattleSimulation) -> EntityId {
        let id = Self::create_animated_artifact(
            game_io,
            simulation,
            ResourcePaths::BATTLE_TRAP_ALERT,
            ResourcePaths::BATTLE_TRAP_ALERT_ANIMATION,
        );

        let spawn_callback = BattleCallback::new(|game_io, _, simulation, _| {
            simulation.play_sound(game_io, &Globals::from_resources(game_io).sfx.trap);
        });

        let _ = simulation
            .entities
            .insert_one(id.into(), SpawnCallback(spawn_callback));

        id
    }

    pub fn create_poof(game_io: &GameIO, simulation: &mut BattleSimulation) -> EntityId {
        Self::create_animated_artifact(
            game_io,
            simulation,
            ResourcePaths::BATTLE_POOF,
            ResourcePaths::BATTLE_POOF_ANIMATION,
        )
    }

    pub fn create_card_poof(
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        parent_entity: EntityId,
    ) -> EntityId {
        let entities = &mut simulation.entities;

        // get the spawn position
        let entity = entities
            .query_one_mut::<&mut Entity>(parent_entity.into())
            .unwrap();
        let mut full_position = entity.full_position();
        full_position.offset.y -= entity.height + 8.0;

        // summon poof
        let poof_id = Artifact::create_poof(game_io, simulation);
        let poof_entity = simulation
            .entities
            .query_one_mut::<&mut Entity>(poof_id.into())
            .unwrap();

        poof_entity.copy_full_position(full_position);
        poof_entity.pending_spawn = true;

        Component::create_float(simulation, poof_id);

        poof_id
    }

    pub fn create_transformation_shine(
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
    ) -> EntityId {
        Self::create_animated_artifact(
            game_io,
            simulation,
            ResourcePaths::BATTLE_SHINE,
            ResourcePaths::BATTLE_SHINE_ANIMATION,
        )
    }
}
