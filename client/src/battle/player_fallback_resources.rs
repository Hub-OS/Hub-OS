use crate::battle::{
    BattleSimulation, Entity, EntityShadow, EntityShadowHidden, Player, PlayerSetup,
    SharedBattleResources,
};
use crate::bindable::{EntityId, SpriteColorMode};
use crate::packages::PlayerPackage;
use crate::render::{Animator, SpriteNode};
use framework::math::Vec2;
use framework::prelude::{Color, GameIO};

use super::{BattleAnimator, BattleMeta};

#[derive(Default)]
pub struct PlayerFallbackResource {
    pub texture_path: String,
    pub animator: Animator,
    pub palette_path: Option<String>,
    pub layer: i32,
    pub offset: Vec2,
    pub use_parent_shader: bool,
}

impl PlayerFallbackResource {
    fn apply_to_sprite_node(&self, game_io: &GameIO, sprite_node: &mut SpriteNode) {
        sprite_node.set_texture(game_io, self.texture_path.clone());
        sprite_node.set_palette(game_io, self.palette_path.clone());
        sprite_node.set_offset(self.offset);
        sprite_node.set_layer(self.layer);
        sprite_node.set_using_parent_shader(self.use_parent_shader);
    }
}

#[derive(Default)]
pub struct PlayerFallbackResources {
    pub base: PlayerFallbackResource,
    pub synced: Vec<PlayerFallbackResource>,
    pub charge_color: Color,
    pub charge_offset: Vec2,
}

impl PlayerFallbackResources {
    pub fn adopt(self, game_io: &GameIO, simulation: &mut BattleSimulation, id: EntityId) {
        let (entity, player) = simulation
            .entities
            .query_one_mut::<(&mut Entity, &mut Player)>(id.into())
            .unwrap();

        let sprite_tree = &mut simulation.sprite_trees[entity.sprite_tree_index];

        // update charge colors and offsets
        player.attack_charge.set_color(self.charge_color);
        (player.attack_charge).update_sprite_offset(sprite_tree, self.charge_offset);
        (player.card_charge).update_sprite_offset(sprite_tree, self.charge_offset);

        // attach sync nodes
        let synced_animator_indices = self
            .synced
            .into_iter()
            .map(|player_resource| {
                let mut sprite_node = SpriteNode::new(game_io, SpriteColorMode::Add);
                player_resource.apply_to_sprite_node(game_io, &mut sprite_node);
                let sprite_index = sprite_tree.insert_root_child(sprite_node);

                let mut battle_animator = BattleAnimator::new();
                battle_animator.set_target(entity.sprite_tree_index, sprite_index);
                let callbacks = battle_animator.copy_from_animator(&player_resource.animator);
                simulation.pending_callbacks.extend(callbacks);
                simulation.animators.insert(battle_animator)
            })
            .collect::<Vec<_>>();

        // adopt texture
        let sprite_node = sprite_tree.root_mut();
        self.base.apply_to_sprite_node(game_io, sprite_node);

        // adopt animator
        let battle_animator = &mut simulation.animators[entity.animator_index];
        let callbacks = battle_animator.copy_from_animator(&self.base.animator);
        simulation.pending_callbacks.extend(callbacks);
        battle_animator.find_and_apply_to_target(&mut simulation.sprite_trees);

        // sync animators
        for index in synced_animator_indices {
            battle_animator.add_synced_animator(index);
        }

        BattleAnimator::sync_animators(
            &mut simulation.animators,
            &mut simulation.sprite_trees,
            &mut simulation.pending_callbacks,
            entity.animator_index,
        );
    }

    pub fn resolve(game_io: &GameIO, package: &PlayerPackage) -> Self {
        // create simulation
        let setup = PlayerSetup {
            package_id: package.package_info.id.clone(),
            ..PlayerSetup::new_empty(0, true)
        };

        let mut meta = BattleMeta {
            player_setups: vec![setup],
            ..BattleMeta::new_with_defaults(game_io, None)
        };
        let mut simulation = BattleSimulation::new(game_io, &meta);

        // load vms
        let mut resources = SharedBattleResources::new(game_io, &meta);
        resources.load_encounter_vms(game_io, &mut simulation, &meta);
        resources.load_player_vms(game_io, &mut simulation, &meta);

        // load player into the simulation
        let setup = meta.player_setups.pop().unwrap();
        let Ok(entity_id) = Player::load(game_io, &resources, &mut simulation, &setup) else {
            return Default::default();
        };

        // resolve shadow sprite tree index before ggathering sync nodes
        type ShadowQuery<'a> = hecs::Without<&'a EntityShadow, &'a EntityShadowHidden>;
        let entities = &mut simulation.entities;

        let shadow_sprite_tree_index = entities
            .query_one_mut::<ShadowQuery>(entity_id.into())
            .ok()
            .map(|shadow| shadow.sprite_tree_index);

        // grab the entitiy
        let (entity, player) = simulation
            .entities
            .query_one_mut::<(&Entity, &Player)>(entity_id.into())
            .unwrap();

        // take the animator
        let battle_animator = simulation.animators.remove(entity.animator_index).unwrap();

        // retain direct synced sprites and the synced shadow sprite
        let synced = battle_animator
            .synced_animators()
            .iter()
            .flat_map(|animator_index| {
                let battle_animator = simulation.animators.remove(*animator_index).unwrap();
                let target_tree_index = battle_animator.target_tree_index()?;

                let is_shadow = Some(target_tree_index) == shadow_sprite_tree_index;

                if target_tree_index != entity.sprite_tree_index && !is_shadow {
                    return None;
                }

                let sprite_tree = simulation.sprite_trees.get(target_tree_index)?;
                let sprite_index = battle_animator.target_sprite_index()?;

                let sprite_tree_node = sprite_tree.get_node(sprite_index)?;

                // avoid non root and indirect sprites
                if sprite_tree_node
                    .parent_index()
                    .is_some_and(|index| index != sprite_tree.root_index())
                {
                    return None;
                }

                let sprite_node = sprite_tree_node.value();

                if !sprite_node.visible() {
                    return None;
                }

                let layer = if is_shadow {
                    i32::MAX
                } else {
                    sprite_node.layer()
                };

                Some(PlayerFallbackResource {
                    texture_path: sprite_node.texture_path().to_string(),
                    animator: battle_animator.take_animator(),
                    palette_path: sprite_node.palette_path().map(String::from),
                    layer,
                    offset: sprite_node.offset(),
                    use_parent_shader: sprite_node.using_parent_shader()
                        || sprite_node.using_root_shader(),
                })
            })
            .collect();

        let Some(sprite_tree) = simulation.sprite_trees.get(entity.sprite_tree_index) else {
            return Default::default();
        };

        let sprite_node = sprite_tree.root();

        Self {
            base: PlayerFallbackResource {
                texture_path: sprite_node.texture_path().to_string(),
                animator: battle_animator.take_animator(),
                palette_path: sprite_node.palette_path().map(str::to_string),
                ..Default::default()
            },
            synced,
            charge_color: player.attack_charge.color(),
            charge_offset: player.attack_charge.sprite_offset(sprite_tree),
        }
    }
}
