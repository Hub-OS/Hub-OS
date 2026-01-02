use super::ui::TextStyle;
use super::{Animator, SpriteColorQueue, SpriteShaderEffect};
use crate::bindable::SpriteColorMode;
use crate::resources::*;
use framework::prelude::*;
use smallvec::SmallVec;
use std::sync::Arc;

pub use crate::structures::{Tree, TreeIndex, TreeNode};

#[derive(Clone)]
pub struct SpriteNode {
    layer: i32,
    palette_path: String,
    palette: Option<Arc<Texture>>,
    texture_path: String,
    sprite: Sprite,
    color_mode: SpriteColorMode, // root node resets every frame
    shader_effect: SpriteShaderEffect,
    using_root_shader: bool,
    using_parent_shader: bool,
    never_flip: bool,
    visible: bool,
}

impl SpriteNode {
    pub fn new(game_io: &GameIO, color_mode: SpriteColorMode) -> Self {
        let assets = &Globals::from_resources(game_io).assets;

        let mut sprite = assets.new_sprite(game_io, ResourcePaths::BLANK);
        sprite.set_color(color_mode.default_color());

        Self {
            layer: 0,
            palette_path: String::new(),
            palette: None,
            texture_path: ResourcePaths::BLANK.to_string(),
            sprite,
            color_mode,
            using_root_shader: false,
            using_parent_shader: false,
            shader_effect: SpriteShaderEffect::Default,
            never_flip: false,
            visible: true,
        }
    }

    pub fn layer(&self) -> i32 {
        self.layer
    }

    /// Negative is closer, positive is farther
    pub fn set_layer(&mut self, layer: i32) {
        self.layer = layer;
    }

    pub fn offset(&self) -> Vec2 {
        self.sprite.position()
    }

    pub fn set_offset(&mut self, offset: Vec2) {
        self.sprite.set_position(offset);
    }

    pub fn origin(&self) -> Vec2 {
        self.sprite.origin()
    }

    pub fn set_origin(&mut self, origin: Vec2) {
        self.sprite.set_origin(origin);
    }

    pub fn apply_animation(&mut self, animator: &Animator) {
        animator.apply(&mut self.sprite);
    }

    pub fn set_frame(&mut self, frame: Rect) {
        self.sprite.set_frame(frame);
    }

    pub fn scale(&self) -> Vec2 {
        self.sprite.scale()
    }

    pub fn set_scale(&mut self, scale: Vec2) {
        self.sprite.set_scale(scale);
    }

    pub fn size(&mut self) -> Vec2 {
        self.sprite.size()
    }

    pub fn set_size(&mut self, size: Vec2) {
        self.sprite.set_size(size);
    }

    pub fn visible(&self) -> bool {
        self.visible
    }

    pub fn set_visible(&mut self, visible: bool) {
        self.visible = visible;
    }

    pub fn never_flip(&self) -> bool {
        self.never_flip
    }

    pub fn set_never_flip(&mut self, never_flip: bool) {
        self.never_flip = never_flip;
    }

    pub fn using_root_shader(&self) -> bool {
        self.using_root_shader
    }

    pub fn set_using_root_shader(&mut self, using_root_shader: bool) {
        self.using_root_shader = using_root_shader;
    }

    pub fn using_parent_shader(&self) -> bool {
        self.using_parent_shader
    }

    pub fn set_using_parent_shader(&mut self, using_root_shader: bool) {
        self.using_parent_shader = using_root_shader;
    }

    pub fn color(&self) -> Color {
        self.sprite.color()
    }

    pub fn set_color(&mut self, color: Color) {
        self.sprite.set_color(color);
    }

    pub fn set_alpha(&mut self, alpha: f32) {
        let mut color = self.sprite.color();
        color.a = alpha;
        self.sprite.set_color(color);
    }

    pub fn color_mode(&self) -> SpriteColorMode {
        self.color_mode
    }

    pub fn set_color_mode(&mut self, mode: SpriteColorMode) {
        self.color_mode = mode;
    }

    pub fn shader_effect(&self) -> SpriteShaderEffect {
        self.shader_effect
    }

    pub fn set_shader_effect(&mut self, effect: SpriteShaderEffect) {
        self.shader_effect = effect;
    }

    pub fn palette_path(&self) -> Option<&str> {
        #[allow(clippy::question_mark)]
        if self.palette.is_none() {
            return None;
        }

        Some(&self.palette_path)
    }

    pub fn palette(&self) -> Option<&Arc<Texture>> {
        self.palette.as_ref()
    }

    pub fn set_palette(&mut self, game_io: &GameIO, path: Option<String>) {
        if let Some(path) = path {
            let assets = &Globals::from_resources(game_io).assets;
            self.palette = Some(assets.texture(game_io, &path));
            self.palette_path = path;
        } else {
            self.palette = None;
            self.palette_path = String::new();
        }
    }

    pub fn texture(&self) -> &Arc<Texture> {
        self.sprite.texture()
    }

    pub fn texture_path(&self) -> &str {
        &self.texture_path
    }

    pub fn set_texture(&mut self, game_io: &GameIO, path: String) {
        let globals = Globals::from_resources(game_io);

        self.sprite
            .set_texture(globals.assets.texture(game_io, &path));

        self.texture_path = path;
    }

    pub fn set_texture_direct(&mut self, texture: Arc<Texture>) {
        self.sprite.set_texture(texture);
        self.texture_path = String::new();
    }

    pub fn sprite(&self) -> &Sprite {
        &self.sprite
    }
}

struct InheritedProperties {
    offset: Vec2,
    flipped: bool,
    scale: Vec2,
}

impl Tree<SpriteNode> {
    pub fn insert_root_text_child(
        &mut self,
        game_io: &GameIO,
        text_style: &TextStyle,
        text: &str,
    ) -> TreeIndex {
        self.insert_text_child(game_io, self.root_index(), text_style, text)
            .unwrap()
    }

    pub fn insert_text_child(
        &mut self,
        game_io: &GameIO,
        parent: TreeIndex,
        text_style: &TextStyle,
        text: &str,
    ) -> Option<TreeIndex> {
        let mut text_node = SpriteNode::new(game_io, SpriteColorMode::Multiply);
        text_node.set_color(text_style.color);
        let text_node_index = self.insert_child(parent, text_node)?;

        let atlas_texture = text_style.glyph_atlas.texture();

        // add shadow
        if text_style.shadow_color.a > 0.0 {
            text_style.iterate(text, |frame, offset, _| {
                let mut char_node = SpriteNode::new(game_io, SpriteColorMode::Multiply);
                char_node.set_color(text_style.shadow_color);
                char_node.set_layer(1);

                char_node.set_texture_direct(atlas_texture.clone());
                frame.apply(&mut char_node.sprite);
                char_node.set_offset(offset + text_style.scale);

                self.insert_child(text_node_index, char_node);
            });
        }

        // add characters
        text_style.iterate(text, |frame, offset, _| {
            let mut char_node = SpriteNode::new(game_io, SpriteColorMode::Multiply);
            char_node.set_using_parent_shader(true);

            char_node.set_texture_direct(atlas_texture.clone());
            frame.apply(&mut char_node.sprite);
            char_node.set_offset(offset);

            self.insert_child(text_node_index, char_node);
        });

        Some(text_node_index)
    }

    pub fn global_position(&self, mut index: TreeIndex) -> Vec2 {
        let mut position = Vec2::ZERO;

        if let Some(node) = self.get_node(index) {
            position += node.value().offset();

            if let Some(parent_index) = node.parent_index() {
                index = parent_index;
            } else {
                return position;
            }
        }

        while let Some(tree_node) = self.get_node(index) {
            let sprite_node = tree_node.value();
            position += sprite_node.offset();
            position += sprite_node.origin();

            if let Some(parent_index) = tree_node.parent_index() {
                index = parent_index;
            } else {
                break;
            }
        }

        position
    }

    pub fn draw(&mut self, sprite_queue: &mut SpriteColorQueue) {
        self.draw_with_offset(sprite_queue, Vec2::ZERO, false);
    }

    pub fn draw_with_offset(
        &mut self,
        sprite_queue: &mut SpriteColorQueue,
        root_offset: Vec2,
        flipped: bool,
    ) {
        let initial_shader_effect = sprite_queue.shader_effect();
        let initial_color_mode = sprite_queue.color_mode();

        let inherit = InheritedProperties {
            offset: root_offset,
            flipped,
            scale: if flipped {
                Vec2::new(-1.0, 1.0)
            } else {
                Vec2::ONE
            },
        };

        self.draw_subtree(sprite_queue, self.root_index(), &inherit);

        sprite_queue.set_shader_effect(initial_shader_effect);
        sprite_queue.set_color_mode(initial_color_mode);
        sprite_queue.set_palette(None);
    }

    fn draw_subtree(
        &mut self,
        sprite_queue: &mut SpriteColorQueue,
        index: TreeIndex,
        inherited: &InheritedProperties,
    ) {
        let tree_node = self.get_node(index).unwrap();
        let sprite_node = tree_node.value();

        if !tree_node.value().visible {
            return;
        }

        let parent_flipped = inherited.flipped;

        // resolve inherited properties for child nodes
        let mut inherited = InheritedProperties {
            offset: inherited.offset + sprite_node.offset() * inherited.scale,
            flipped: inherited.flipped && !sprite_node.never_flip,
            scale: inherited.scale * sprite_node.scale(),
        };

        if parent_flipped && !inherited.flipped {
            inherited.scale.x *= -1.0;
        }

        // gather child indices
        type IndexVec<'a> = SmallVec<[TreeIndex; 5]>;
        let mut children = IndexVec::with_capacity(tree_node.children().len());
        children.extend(tree_node.children().iter().cloned());
        children.sort_by_cached_key(|i| -self.get(*i).unwrap().layer);

        let positive_end = children
            .iter()
            .take_while(|i| self.get(**i).unwrap().layer > 0)
            .count();

        // draw children with layer > 0
        for &child_index in &children[..positive_end] {
            self.draw_subtree(sprite_queue, child_index, &inherited);
        }

        // modify the sprite using inherited values
        let sprite_node = self.get_mut(index).unwrap();
        let original_offset = sprite_node.offset();
        let original_scale = sprite_node.scale();

        sprite_node.sprite.set_position(inherited.offset);
        sprite_node.sprite.set_scale(inherited.scale);

        // draw our node
        self.draw_index(sprite_queue, index);

        // reapply local properties
        let sprite_node = self.get_mut(index).unwrap();
        sprite_node.set_offset(original_offset);
        sprite_node.set_scale(original_scale);

        // draw children with layer <= 0
        for &child_index in &children[positive_end..] {
            self.draw_subtree(sprite_queue, child_index, &inherited);
        }
    }

    fn draw_index(&mut self, sprite_queue: &mut SpriteColorQueue, index: TreeIndex) {
        let tree_node = self.get_node_mut(index).unwrap();
        let parent_index = tree_node.parent_index();
        let mut sprite_node = tree_node.value_mut();

        // resolve shader
        let original_color = sprite_node.color();

        if sprite_node.using_root_shader {
            let root_sprite_node = self.root();
            let color = root_sprite_node.color();

            sprite_queue.set_color_mode(root_sprite_node.color_mode());
            sprite_queue.set_shader_effect(root_sprite_node.shader_effect);

            sprite_node = self.get_mut(index).unwrap();
            sprite_node.set_color(color);
        } else if sprite_node.using_parent_shader && parent_index.is_some() {
            let parent_sprite_node = self.get(parent_index.unwrap()).unwrap();
            let color = parent_sprite_node.color();

            sprite_queue.set_color_mode(parent_sprite_node.color_mode());
            sprite_queue.set_shader_effect(parent_sprite_node.shader_effect);

            sprite_node = self.get_mut(index).unwrap();
            sprite_node.set_color(color);
        } else {
            sprite_queue.set_color_mode(sprite_node.color_mode());
            sprite_queue.set_shader_effect(sprite_node.shader_effect);
        }

        sprite_queue.set_palette(sprite_node.palette.clone());

        // finally drawing the sprite
        sprite_queue.draw_sprite(sprite_node.sprite());

        sprite_node.set_color(original_color);
    }
}
