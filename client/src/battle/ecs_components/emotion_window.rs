use crate::battle::{BattleSimulation, PlayerSetup};
use crate::bindable::{EntityId, GenerationalIndex, SpriteColorMode};
use crate::packages::PlayerPackage;
use crate::render::{Animator, SpriteNode, Tree};
use crate::{Globals, ResourcePaths};
use framework::common::GameIO;
use packets::structures::Emotion;
use std::sync::Arc;

#[derive(Clone)]
pub struct EmotionWindow {
    emotion: Emotion,
    animation_path: Arc<str>,
    animator: Animator,
    sprite_tree_index: GenerationalIndex,
}

impl EmotionWindow {
    pub(crate) fn build_new(
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        player_package: &PlayerPackage,
        setup: &PlayerSetup,
        entity_id: EntityId,
    ) {
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;

        // load emotions assets with defaults to prevent warnings
        let (texture_path, animation_path): (&str, &str) =
            if let Some(pair) = player_package.emotions_paths.as_ref() {
                (&pair.texture, &pair.animation)
            } else {
                (ResourcePaths::BLANK, ResourcePaths::BLANK)
            };

        let mut emotion = setup.emotion.clone();

        // load animation
        let mut animator = Animator::load_new(assets, animation_path);

        if !animator.has_state(emotion.as_str()) {
            // use the default emotion if the provided emotion isn't supported
            emotion = Emotion::default();
        }

        animator.set_state(emotion.as_str());

        // load sprite
        let mut root_node = SpriteNode::new(game_io, SpriteColorMode::Add);
        root_node.set_texture(game_io, texture_path.to_string());
        root_node.apply_animation(&animator);

        let sprite_tree = Tree::new(root_node);
        let sprite_tree_index = simulation.sprite_trees.insert(sprite_tree);

        // build emotion window
        let emotion_window = Self {
            emotion,
            animation_path: animation_path.into(),
            animator,
            sprite_tree_index,
        };

        let entities = &mut simulation.entities;
        let _ = entities.insert_one(entity_id.into(), emotion_window);
    }

    pub fn sprite_tree_index(&self) -> GenerationalIndex {
        self.sprite_tree_index
    }

    pub fn animation_path(&self) -> &str {
        &self.animation_path
    }

    pub fn animator(&self) -> &Animator {
        &self.animator
    }

    pub fn load_animation(&mut self, game_io: &GameIO, path: String) {
        let globals = Globals::from_resources(game_io);
        self.animator.load(&globals.assets, &path);
        self.animation_path = path.into();

        if !self.animator.has_state(self.emotion.as_str()) {
            self.emotion = Emotion::default();
            self.animator.set_state(self.emotion.as_str());
        }
    }

    pub fn emotions(&self) -> impl Iterator<Item = &str> {
        self.animator.iter_states().map(|(s, _)| s)
    }

    pub fn emotion(&self) -> &Emotion {
        &self.emotion
    }

    pub fn has_emotion(&self, emotion: &Emotion) -> bool {
        self.animator.has_state(emotion.as_str())
    }

    pub fn set_emotion(&mut self, emotion: Emotion) {
        if self.animator.has_state(emotion.as_str()) {
            // only apply the emotion if it's supported
            self.animator.set_state(emotion.as_str());
            self.emotion = emotion;
        }
    }

    pub fn update(&mut self) {
        self.animator.update();
    }

    pub fn delete(simulation: &mut BattleSimulation, id: EntityId) {
        let Ok(emotion_window) = simulation.entities.remove_one::<Self>(id.into()) else {
            return;
        };

        let tree_index = emotion_window.sprite_tree_index;
        simulation.sprite_trees.remove(tree_index);
    }
}
