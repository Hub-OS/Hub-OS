use crate::bindable::Element;
use crate::packages::*;
use crate::render::ui::*;
use crate::render::*;
use crate::resources::Globals;
use crate::resources::*;
use framework::prelude::*;
use serde::{Deserialize, Serialize};

#[derive(Clone, PartialEq, Eq, Serialize, Deserialize, Hash)]
pub struct Card {
    pub package_id: PackageId,
    pub code: String,
}

impl Card {
    // used in netplay, luckily we shouldnt see what remotes have, so using local namespace is fine
    pub fn draw_icon(&self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue, position: Vec2) {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;
        let package_manager = &globals.card_packages;
        let ns = PackageNamespace::Local;
        let package = package_manager.package_or_override(ns, &self.package_id);

        let icon_texture = match package {
            Some(package) => {
                let icon_texture = assets.texture(game_io, &package.icon_texture_path);

                if icon_texture.size() == UVec2::new(14, 14) {
                    icon_texture
                } else {
                    assets.texture(game_io, ResourcePaths::CARD_ICON_MISSING)
                }
            }
            None => assets.texture(game_io, ResourcePaths::CARD_ICON_MISSING),
        };

        let mut sprite = Sprite::new(game_io, icon_texture);
        sprite.set_position(position);
        sprite_queue.draw_sprite(&sprite);
    }

    pub fn draw_preview_title(
        &self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        position: Vec2,
    ) {
        let globals = game_io.resource::<Globals>().unwrap();
        let package_manager = &globals.card_packages;
        let name = package_manager
            .package_or_override(PackageNamespace::Local, &self.package_id)
            .map(|package| package.card_properties.short_name.as_ref())
            .unwrap_or("?????");

        let mut text_style = TextStyle::new_monospace(game_io, FontStyle::Thick);
        text_style.letter_spacing = 2.0;
        text_style.shadow_color = TEXT_DARK_SHADOW_COLOR;
        text_style.bounds.set_position(position);
        text_style.bounds.x -= 27.0;
        text_style.bounds.y -= text_style.line_height() + text_style.line_spacing;

        text_style.draw(game_io, sprite_queue, name);
    }

    pub fn draw_preview(
        &self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        position: Vec2,
        scale: f32,
    ) {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;
        let package_manager = &globals.card_packages;

        let (preview_texture_path, element, secondary_element, damage);

        if let Some(package) =
            package_manager.package_or_override(PackageNamespace::Local, &self.package_id)
        {
            preview_texture_path = package.preview_texture_path.as_str();
            element = package.card_properties.element;
            secondary_element = package.card_properties.secondary_element;
            damage = package.card_properties.damage;
        } else {
            preview_texture_path = "";
            element = Element::None;
            secondary_element = Element::None;
            damage = 0;
        }

        const PREVIEW_OFFSET: Vec2 = Vec2::new(0.0, 24.0);
        const ELEMENT_OFFSET: Vec2 = Vec2::new(-23.0, 49.0);
        const ELEMENT2_OFFSET: Vec2 = Vec2::new(-8.0, 49.0);
        const CODE_OFFSET: Vec2 = Vec2::new(-31.0, 51.0);
        const DAMAGE_OFFSET: Vec2 = Vec2::new(31.0, 51.0);

        let scale = Vec2::new(scale, 1.0);

        // preview
        let mut preview_texture = assets.texture(game_io, preview_texture_path);

        if preview_texture.size() == UVec2::new(1, 1) {
            preview_texture = assets.texture(game_io, ResourcePaths::CARD_PREVIEW_MISSING);
        }

        let mut sprite = Sprite::new(game_io, preview_texture);
        sprite.set_origin(-PREVIEW_OFFSET + sprite.size() * 0.5);
        sprite.set_position(position);
        sprite.set_scale(scale);
        sprite_queue.draw_sprite(&sprite);

        // secondary_element
        if secondary_element != Element::None {
            let mut element_sprite = ElementSprite::new(game_io, secondary_element);
            element_sprite.set_origin(-ELEMENT2_OFFSET);
            element_sprite.set_position(position);
            element_sprite.set_scale(scale);
            sprite_queue.draw_sprite(&element_sprite);
        }

        // element
        let mut element_sprite = ElementSprite::new(game_io, element);
        element_sprite.set_origin(-ELEMENT_OFFSET);
        element_sprite.set_position(position);
        element_sprite.set_scale(scale);
        sprite_queue.draw_sprite(&element_sprite);

        // code
        let mut label = TextStyle::new(game_io, FontStyle::Thick);
        label.letter_spacing = 2.0;
        label.scale = scale;
        label.bounds.set_position(CODE_OFFSET * scale + position);

        label.color = Color::YELLOW;
        label.draw(game_io, sprite_queue, &self.code);

        // damage
        if damage > 0 {
            label.color = Color::WHITE;
            let text = format!("{:>3}", damage);

            let damage_width = label.measure(&text).size.x;
            let mut damage_offset = DAMAGE_OFFSET + Vec2::new(-damage_width, 0.0);
            damage_offset.x -= DAMAGE_OFFSET.x - DAMAGE_OFFSET.x * scale.x;

            label.bounds.set_position(damage_offset + position);
            label.draw(game_io, sprite_queue, &text);
        }
    }

    pub fn draw_list_item(
        &self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        position: Vec2,
        color: Color,
    ) {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;
        let package_manager = &globals.card_packages;

        let (icon_texture_path, short_name, element, limit);

        if let Some(package) =
            package_manager.package_or_override(PackageNamespace::Local, &self.package_id)
        {
            icon_texture_path = package.icon_texture_path.as_str();
            short_name = package.card_properties.short_name.as_ref();
            element = package.card_properties.element;
            limit = package.limit;
        } else {
            icon_texture_path = ResourcePaths::CARD_ICON_MISSING;
            short_name = "?????";
            element = Element::None;
            limit = 0;
        };

        const ICON_OFFSET: Vec2 = Vec2::new(2.0, 1.0);
        const NAME_OFFSET: Vec2 = Vec2::new(18.0, 3.0);
        const ELEMENT_OFFSET: Vec2 = Vec2::new(75.0, 1.0);
        const CODE_OFFSET: Vec2 = Vec2::new(91.0, 3.0);
        const LIMIT_OFFSET: Vec2 = Vec2::new(101.0, 1.0);
        const LIM_OFFSET: Vec2 = Vec2::new(101.0, 8.0);

        // icon
        let mut icon_texture = assets.texture(game_io, icon_texture_path);

        if icon_texture.size() != UVec2::new(14, 14) {
            icon_texture = assets.texture(game_io, ResourcePaths::CARD_ICON_MISSING);
        }

        let mut sprite = Sprite::new(game_io, icon_texture);
        sprite.set_position(ICON_OFFSET + position);
        sprite_queue.draw_sprite(&sprite);

        // text style
        let mut label = TextStyle::new(game_io, FontStyle::Thick);
        label.shadow_color = TEXT_DARK_SHADOW_COLOR;
        label.color = color;

        // name
        label.bounds.set_position(NAME_OFFSET + position);
        label.draw(game_io, sprite_queue, short_name);

        // element
        let mut element_sprite = ElementSprite::new(game_io, element);
        element_sprite.set_position(ELEMENT_OFFSET + position);
        sprite_queue.draw_sprite(&element_sprite);

        // code
        label.bounds.set_position(CODE_OFFSET + position);
        label.color = Color::WHITE;
        label.draw(game_io, sprite_queue, &self.code);

        // limit
        label.font_style = FontStyle::Wide;
        label.bounds.set_position(LIMIT_OFFSET + position);
        let text = format!("{limit:>2}");
        label.color = Color::from((247, 214, 99, 255));
        label.draw(game_io, sprite_queue, &text);

        label.bounds.set_position(LIM_OFFSET + position);
        label.color = Color::WHITE;
        label.draw(game_io, sprite_queue, "LM");
    }
}
