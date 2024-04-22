use crate::battle::BattleSimulation;
use crate::bindable::{EntityId, SpriteColorMode};
use crate::render::SpriteNode;
use crate::structures::{Tree, TreeIndex};
use framework::common::GameIO;

#[derive(Clone)]
pub struct EntityShadowVisible;

#[derive(Clone)]
pub struct EntityShadow {
    pub sprite_tree_index: TreeIndex,
}

impl EntityShadow {
    pub fn set_visible(simulation: &mut BattleSimulation, entity_id: EntityId, visible: bool) {
        let entities = &mut simulation.entities;

        if visible {
            let _ = entities.insert_one(entity_id.into(), EntityShadowVisible);
        } else {
            let _ = entities.remove_one::<EntityShadowVisible>(entity_id.into());
        }
    }

    pub fn set(
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
        path: String,
    ) {
        let entities = &mut simulation.entities;
        let sprite_trees = &mut simulation.sprite_trees;

        match entities.query_one_mut::<&mut EntityShadow>(entity_id.into()) {
            Ok(shadow) => {
                // update existing shadow

                if let Some(sprite_tree) = sprite_trees.get_mut(shadow.sprite_tree_index) {
                    let sprite_node = sprite_tree.root_mut();
                    sprite_node.set_texture(game_io, path);

                    let shadow_size = sprite_node.size();
                    sprite_node.set_origin(shadow_size * 0.5);
                }
            }
            Err(hecs::QueryOneError::Unsatisfied) => {
                // create a new one

                let mut sprite_node = SpriteNode::new(game_io, SpriteColorMode::Add);
                sprite_node.set_texture(game_io, path);

                let shadow_size = sprite_node.size();
                sprite_node.set_origin(shadow_size * 0.5);

                let sprite_tree = Tree::<SpriteNode>::new(sprite_node);
                let shadow = Self {
                    sprite_tree_index: sprite_trees.insert(sprite_tree),
                };

                let _ = simulation.entities.insert_one(entity_id.into(), shadow);
            }
            _ => {}
        };
    }

    pub fn delete(simulation: &mut BattleSimulation, entity_id: EntityId) {
        let entities = &mut simulation.entities;

        if let Ok(shadow) = entities.query_one_mut::<&EntityShadow>(entity_id.into()) {
            simulation.sprite_trees.remove(shadow.sprite_tree_index);
        }
    }
}
