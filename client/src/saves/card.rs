use crate::bindable::Element;
use crate::packages::*;
use crate::render::ui::*;
use crate::render::*;
use crate::resources::Globals;
use crate::resources::*;
use framework::prelude::*;
use serde::{Deserialize, Serialize};

#[derive(Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct Card {
    pub package_id: String,
    pub code: String,
}

impl Card {
    // used in pvp, luckily we shouldnt see what the opponent has, so using server namespace is fine
    pub fn draw_icon(
        &self,
        game_io: &GameIO<Globals>,
        sprite_queue: &mut SpriteColorQueue,
        position: Vec2,
    ) {
        let globals = game_io.globals();
        let assets = &globals.assets;
        let package_manager = &globals.card_packages;
        let package =
            package_manager.package_or_fallback(PackageNamespace::Server, &self.package_id);

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

        let mut sprite = Sprite::new(icon_texture, globals.default_sampler.clone());
        sprite.set_position(position);
        sprite_queue.draw_sprite(&sprite);
    }

    pub fn draw_preview(
        &self,
        game_io: &GameIO<Globals>,
        sprite_queue: &mut SpriteColorQueue,
        position: Vec2,
        scale: f32,
    ) {
        let globals = game_io.globals();
        let assets = &globals.assets;
        let package_manager = &globals.card_packages;
        let package =
            package_manager.package_or_fallback(PackageNamespace::Server, &self.package_id);

        let package = match package {
            Some(package) => package,
            None => return,
        };

        const PREVIEW_OFFSET: Vec2 = Vec2::new(0.0, 24.0);
        const ELEMENT_OFFSET: Vec2 = Vec2::new(-20.0, 49.0);
        const ELEMENT2_OFFSET: Vec2 = Vec2::new(-9.0, 49.0);
        const CODE_OFFSET: Vec2 = Vec2::new(-28.0, 51.0);
        const DAMAGE_OFFSET: Vec2 = Vec2::new(28.0, 51.0);

        let scale = Vec2::new(scale, 1.0);

        // preview
        let mut preview_texture = assets.texture(game_io, &package.preview_texture_path);

        if preview_texture.size() == UVec2::new(1, 1) {
            preview_texture = assets.texture(game_io, ResourcePaths::CARD_PREVIEW_MISSING);
        }

        let mut sprite = Sprite::new(preview_texture, globals.default_sampler.clone());
        sprite.set_origin(-PREVIEW_OFFSET + sprite.size() * 0.5);
        sprite.set_position(position);
        sprite.set_scale(scale);
        sprite_queue.draw_sprite(&sprite);

        // secondary_element
        if package.card_properties.secondary_element != Element::None {
            let mut element_sprite =
                ElementSprite::new(game_io, package.card_properties.secondary_element);
            element_sprite.set_origin(-ELEMENT2_OFFSET);
            element_sprite.set_position(position);
            element_sprite.set_scale(scale);
            sprite_queue.draw_sprite(&element_sprite);
        }

        // element
        let mut element_sprite = ElementSprite::new(game_io, package.card_properties.element);
        element_sprite.set_origin(-ELEMENT_OFFSET);
        element_sprite.set_position(position);
        element_sprite.set_scale(scale);
        sprite_queue.draw_sprite(&element_sprite);

        // code
        let mut label = TextStyle::new(game_io, FontStyle::Thick);
        label.scale = scale;
        label.bounds.set_position(CODE_OFFSET * scale + position);

        label.color = Color::YELLOW;
        label.draw(game_io, sprite_queue, &self.code);

        // damage
        label.color = Color::WHITE;
        let text = format!("{:>3}", package.card_properties.damage);

        let damage_width = label.measure(&text).size.x;
        let mut damage_offset = DAMAGE_OFFSET + Vec2::new(-damage_width, 0.0);
        damage_offset.x -= DAMAGE_OFFSET.x - DAMAGE_OFFSET.x * scale.x;

        label.bounds.set_position(damage_offset + position);
        label.draw(game_io, sprite_queue, &text);
    }

    pub fn draw_list_item(
        &self,
        game_io: &GameIO<Globals>,
        sprite_queue: &mut SpriteColorQueue,
        position: Vec2,
    ) {
        let globals = game_io.globals();
        let assets = &globals.assets;
        let package_manager = &globals.card_packages;
        let package =
            package_manager.package_or_fallback(PackageNamespace::Server, &self.package_id);

        let package = match package {
            Some(package) => package,
            None => return,
        };

        const ICON_OFFSET: Vec2 = Vec2::new(2.0, 1.0);
        const NAME_OFFSET: Vec2 = Vec2::new(18.0, 3.0);
        const ELEMENT_OFFSET: Vec2 = Vec2::new(75.0, 1.0);
        const CODE_OFFSET: Vec2 = Vec2::new(91.0, 3.0);
        const LIMIT_OFFSET: Vec2 = Vec2::new(101.0, 1.0);
        const LIM_OFFSET: Vec2 = Vec2::new(101.0, 8.0);

        // icon
        let mut icon_texture = assets.texture(game_io, &package.icon_texture_path);

        if icon_texture.size() != UVec2::new(14, 14) {
            icon_texture = assets.texture(game_io, ResourcePaths::CARD_ICON_MISSING);
        }

        let mut sprite = Sprite::new(icon_texture, globals.default_sampler.clone());
        sprite.set_position(ICON_OFFSET + position);
        sprite_queue.draw_sprite(&sprite);

        // name
        let mut label = TextStyle::new(game_io, FontStyle::Thick);
        label.shadow_color = TEXT_DARK_SHADOW_COLOR;
        label.bounds.set_position(NAME_OFFSET + position);
        label.draw(game_io, sprite_queue, &package.card_properties.short_name);

        // element
        let mut element_sprite = ElementSprite::new(game_io, package.card_properties.element);
        element_sprite.set_position(ELEMENT_OFFSET + position);
        sprite_queue.draw_sprite(&element_sprite);

        // code
        label.bounds.set_position(CODE_OFFSET + position);
        label.draw(game_io, sprite_queue, &self.code);

        // limit
        label.font_style = FontStyle::Wide;
        label.bounds.set_position(LIMIT_OFFSET + position);
        let text = format!("{:>2}", package.card_properties.limit);
        label.color = Color::from((247, 214, 99, 255));
        label.draw(game_io, sprite_queue, &text);

        label.bounds.set_position(LIM_OFFSET + position);
        label.color = Color::WHITE;
        label.draw(game_io, sprite_queue, "LM");
    }
}
