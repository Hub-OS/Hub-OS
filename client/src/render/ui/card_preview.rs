use crate::ResourcePaths;
use crate::bindable::Element;
use crate::packages::PackageNamespace;
use crate::render::SpriteColorQueue;
use crate::render::ui::{ElementSprite, FontName, TextStyle};
use crate::resources::{AssetManager, Globals, TEXT_DARK_SHADOW_COLOR};
use crate::saves::Card;
use framework::prelude::*;

pub struct CardPreview<'a> {
    pub namespace: PackageNamespace,
    pub card: &'a Card,
    pub position: Vec2,
    pub scale: f32,
    pub damage_override: Option<i32>,
}

impl<'a> CardPreview<'a> {
    pub fn new(card: &'a Card, position: Vec2, scale: f32) -> Self {
        Self {
            namespace: PackageNamespace::Local,
            card,
            position,
            scale,
            damage_override: None,
        }
    }

    pub fn draw_title(&self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        let globals = Globals::from_resources(game_io);
        let package_manager = &globals.card_packages;
        let name = package_manager
            .package_or_fallback(self.namespace, &self.card.package_id)
            .map(|package| package.card_properties.short_name.as_ref())
            .unwrap_or("?????");

        let mut text_style = TextStyle::new_monospace(game_io, FontName::Thick);
        text_style.letter_spacing = 2.0;
        text_style.shadow_color = TEXT_DARK_SHADOW_COLOR;
        text_style.bounds.set_position(self.position);
        text_style.bounds.x -= 27.0;
        text_style.bounds.y -= text_style.line_height() + text_style.line_spacing;

        text_style.draw(game_io, sprite_queue, name);
    }

    pub fn draw(&self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;
        let package_manager = &globals.card_packages;

        let (preview_texture_path, element, secondary_element, conceal_damage, damage, can_boost);

        if let Some(package) =
            package_manager.package_or_fallback(self.namespace, &self.card.package_id)
        {
            preview_texture_path = package.preview_texture_path.as_str();
            element = package.card_properties.element;
            secondary_element = package.card_properties.secondary_element;
            conceal_damage =
                package.card_properties.dynamic_damage && self.damage_override.is_none();
            damage = self
                .damage_override
                .unwrap_or(package.card_properties.damage);
            can_boost = package.card_properties.can_boost;
        } else {
            preview_texture_path = "";
            element = Element::None;
            secondary_element = Element::None;
            conceal_damage = false;
            damage = self.damage_override.unwrap_or_default();
            can_boost = true;
        }

        const PREVIEW_OFFSET: Vec2 = Vec2::new(0.0, 24.0);
        const ELEMENT_OFFSET: Vec2 = Vec2::new(-23.0, 49.0);
        const ELEMENT2_OFFSET: Vec2 = Vec2::new(-8.0, 49.0);
        const CODE_OFFSET: Vec2 = Vec2::new(-31.0, 51.0);
        const DAMAGE_OFFSET: Vec2 = Vec2::new(31.0, 51.0);

        let scale = Vec2::new(self.scale, 1.0);

        // preview
        let mut preview_texture = assets.texture(game_io, preview_texture_path);

        if preview_texture.size() == UVec2::new(1, 1) {
            preview_texture = assets.texture(game_io, ResourcePaths::CARD_PREVIEW_MISSING);
        }

        let mut sprite = Sprite::new(game_io, preview_texture);
        sprite.set_origin(-PREVIEW_OFFSET + sprite.size() * 0.5);
        sprite.set_position(self.position);
        sprite.set_scale(scale);
        sprite_queue.draw_sprite(&sprite);

        // secondary_element
        if secondary_element != Element::None {
            let mut element_sprite = ElementSprite::new(game_io, secondary_element);
            element_sprite.set_origin(-ELEMENT2_OFFSET);
            element_sprite.set_position(self.position);
            element_sprite.set_scale(scale);
            sprite_queue.draw_sprite(&element_sprite);
        }

        // element
        let mut element_sprite = ElementSprite::new(game_io, element);
        element_sprite.set_origin(-ELEMENT_OFFSET);
        element_sprite.set_position(self.position);
        element_sprite.set_scale(scale);
        sprite_queue.draw_sprite(&element_sprite);

        // code
        let mut label = TextStyle::new(game_io, FontName::Thick);
        label.letter_spacing = 2.0;
        label.scale = scale;
        label
            .bounds
            .set_position(CODE_OFFSET * scale + self.position);

        label.color = Color::YELLOW;
        label.draw(game_io, sprite_queue, &self.card.code);

        // damage
        if conceal_damage || damage > 0 {
            label.color = Color::WHITE;
            let text = if conceal_damage {
                "???"
            } else {
                &damage.to_string()
            };

            let damage_size = label.measure(text).size;
            let mut damage_offset = DAMAGE_OFFSET + Vec2::new(-damage_size.x, 0.0);
            damage_offset.x -= DAMAGE_OFFSET.x - DAMAGE_OFFSET.x * scale.x;

            label.bounds.set_position(damage_offset + self.position);
            label.draw(game_io, sprite_queue, text);

            if !can_boost {
                // underline non boostable chips
                let globals = Globals::from_resources(game_io);

                let mut line = globals.assets.new_sprite(game_io, ResourcePaths::PIXEL);
                line.set_position(label.bounds.position() + Vec2::new(0.0, damage_size.y + 1.0));
                line.set_width(damage_size.x);
                sprite_queue.draw_sprite(&line);
            }
        }
    }
}
