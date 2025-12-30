use crate::battle::{BattleAnimator, BattleSimulation, Entity};
use crate::bindable::{EntityId, GenerationalIndex, SpriteColorMode};
use crate::render::{SpriteColorQueue, SpriteNode};
use crate::structures::{SlotMap, Tree, TreeIndex};
use framework::prelude::*;

#[derive(Clone)]
pub struct EntityShadowHidden;

#[derive(Clone)]
pub struct EntityShadow {
    pub sprite_tree_index: TreeIndex,
    pub animator_index: Option<GenerationalIndex>,
}

impl EntityShadow {
    pub fn set_visible(simulation: &mut BattleSimulation, entity_id: EntityId, visible: bool) {
        let entities = &mut simulation.entities;

        if visible {
            let _ = entities.remove_one::<EntityShadowHidden>(entity_id.into());
        } else {
            let _ = entities.insert_one(entity_id.into(), EntityShadowHidden);
        }
    }

    pub fn set(
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
        texture_path: String,
        animation_path: Option<String>,
    ) {
        let entities = &mut simulation.entities;
        let sprite_trees = &mut simulation.sprite_trees;

        let Ok((entity, shadow)) =
            entities.query_one_mut::<(&Entity, Option<&mut EntityShadow>)>(entity_id.into())
        else {
            return;
        };

        if let Some(shadow) = shadow {
            // update existing shadow

            if let Some(sprite_tree) = sprite_trees.get_mut(shadow.sprite_tree_index) {
                let sprite_node = sprite_tree.root_mut();
                sprite_node.set_texture(game_io, texture_path);

                let shadow_size = sprite_node.size();
                sprite_node.set_origin(shadow_size * 0.5);
            }

            if let Some(path) = animation_path {
                if let Some(animator) = shadow
                    .animator_index
                    .and_then(|i| simulation.animators.get_mut(i))
                {
                    // reuse existing animator
                    // shadows can't have callbacks, so we'll just ignore them
                    let _ = animator.load(game_io, &path);
                } else {
                    // create a new animator
                    let mut animator = BattleAnimator::new();
                    animator.set_target(shadow.sprite_tree_index, GenerationalIndex::tree_root());

                    let _ = animator.load(game_io, &path);

                    let index = simulation.animators.insert(animator);
                    shadow.animator_index = Some(index);

                    if let Some(animator) = simulation.animators.get_mut(entity.animator_index) {
                        animator.add_synced_animator(index);
                    }
                }
            } else if let Some(index) = shadow.animator_index.take() {
                // delete animator
                simulation.animators.remove(index);

                if let Some(animator) = simulation.animators.get_mut(entity.animator_index) {
                    animator.remove_synced_animator(index);
                }
            }
        } else {
            // create a new one

            let mut sprite_node = SpriteNode::new(game_io, SpriteColorMode::Add);
            sprite_node.set_texture(game_io, texture_path);

            let shadow_size = sprite_node.size();
            sprite_node.set_origin(shadow_size * 0.5);

            let sprite_tree = Tree::<SpriteNode>::new(sprite_node);
            let sprite_tree_index = sprite_trees.insert(sprite_tree);

            let animator_index = animation_path.map(|path| {
                let mut animator = BattleAnimator::new();
                animator.set_target(sprite_tree_index, GenerationalIndex::tree_root());

                let _ = animator.load(game_io, &path);
                let index = simulation.animators.insert(animator);

                if let Some(animator) = simulation.animators.get_mut(entity.animator_index) {
                    animator.add_synced_animator(index);
                }

                index
            });

            let shadow = Self {
                sprite_tree_index,
                animator_index,
            };

            let _ = simulation.entities.insert_one(entity_id.into(), shadow);
        }
    }

    pub fn delete(simulation: &mut BattleSimulation, entity_id: EntityId) {
        let entities = &mut simulation.entities;

        let Ok(shadow) = entities.query_one_mut::<&EntityShadow>(entity_id.into()) else {
            return;
        };

        simulation.sprite_trees.remove(shadow.sprite_tree_index);

        if let Some(index) = shadow.animator_index {
            simulation.animators.remove(index);
        }
    }

    pub fn draw(
        &self,
        sprite_queue: &mut SpriteColorQueue,
        sprite_trees: &mut SlotMap<Tree<SpriteNode>>,
        entity: &Entity,
        position: Vec2,
        flipped: bool,
    ) {
        let base_sprite = sprite_trees
            .get_mut(entity.sprite_tree_index)
            .unwrap()
            .root_mut();
        let color_mode = base_sprite.color_mode();
        let color = base_sprite.color();
        let shader_effect = base_sprite.shader_effect();
        let scale = base_sprite.scale();

        let shadow_tree = sprite_trees.get_mut(self.sprite_tree_index).unwrap();

        let sprite = shadow_tree.root_mut();
        sprite.set_color_mode(color_mode);
        sprite.set_color(color);
        sprite.set_shader_effect(shader_effect);
        sprite.set_scale(scale);
        shadow_tree.draw_with_offset(sprite_queue, position, flipped);
    }
}
