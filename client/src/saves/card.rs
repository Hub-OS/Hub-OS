use crate::bindable::{CardClass, Element};
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

impl std::fmt::Debug for Card {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{} {}", self.package_id, self.code)
    }
}

impl Card {
    pub fn draw_recipe_preview(
        &self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        namespace: PackageNamespace,
        position: Vec2,
        scale: f32,
        recipe_index: usize,
    ) {
        let globals = Globals::from_resources(game_io);
        let package_manager = &globals.card_packages;

        let Some(package) = package_manager.package_or_fallback(namespace, &self.package_id) else {
            return;
        };

        if package.recipes.is_empty() {
            return;
        }

        let len = package.recipes.len();
        let recipe_index = recipe_index % len;

        fn fetch_ingredient_name<'a>(
            package_manager: &'a PackageManager<CardPackage>,
            namespace: PackageNamespace,
            id_type: CardRecipeIdType,
            id: &'a str,
        ) -> &'a str {
            match id_type {
                CardRecipeIdType::Name => id,
                CardRecipeIdType::Id => {
                    if let Some(ingredient_package) =
                        package_manager.package_or_fallback(namespace, &PackageId::from(id))
                    {
                        &ingredient_package.card_properties.short_name
                    } else {
                        "?????"
                    }
                }
            }
        }

        let recipe = &package.recipes[recipe_index];
        let mut text_style = TextStyle::new_monospace(game_io, FontName::Thick);
        text_style.color = Color::from_rgb_u8s(0x48, 0xF8, 0x08);
        text_style.bounds.set_position(position);
        text_style.scale = Vec2::new(scale, 1.0);
        text_style.line_spacing = 3.0;

        let whitespace_width = text_style.measure_grapheme(" ").x + text_style.letter_spacing;
        let top_left = position + Vec2::new(-whitespace_width * 4.5, 0.0);

        let mut line_position = top_left;
        let line_height = text_style.line_height();

        match recipe {
            CardRecipe::CodeSequence { id_type, id, codes } => {
                let name = fetch_ingredient_name(package_manager, namespace, *id_type, id);
                let code_offset = Vec2::new(whitespace_width * 8.0, 0.0);

                for code in codes {
                    text_style.bounds.set_position(line_position);
                    text_style.draw(game_io, sprite_queue, name);
                    text_style.bounds.set_position(line_position + code_offset);
                    text_style.draw(game_io, sprite_queue, code);

                    line_position.y += line_height;
                }
            }
            CardRecipe::MixSequence { mix, .. } => {
                for (id_type, id) in mix {
                    let name = fetch_ingredient_name(package_manager, namespace, *id_type, id);

                    text_style.bounds.set_position(line_position);
                    text_style.draw(game_io, sprite_queue, name);

                    line_position.y += line_height;
                }
            }
        }

        // render bottom portion unscaled
        text_style.scale = Vec2::ONE;

        let whitespace_width = text_style.measure_grapheme(" ").x + text_style.letter_spacing;
        let top_left = position + Vec2::new(-whitespace_width * 4.5, 0.0);

        // render page number
        let offset = Vec2::new(0.0, line_height * 4.0);
        text_style.bounds.set_position(top_left + offset);
        text_style.color = Color::WHITE;

        let page_text = format!("{}/{len}", recipe_index + 1);
        text_style.draw(game_io, sprite_queue, &page_text);
    }

    pub fn draw_list_item(
        &self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        position: Vec2,
        color: Color,
    ) {
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;
        let package_manager = &globals.card_packages;

        let (icon_texture_path, short_name, element, limit, card_class);

        if let Some(package) =
            package_manager.package_or_fallback(PackageNamespace::Local, &self.package_id)
        {
            icon_texture_path = package.icon_texture_path.as_str();
            short_name = package.card_properties.short_name.as_ref();
            element = package.card_properties.element;
            limit = package.limit;
            card_class = package.card_properties.card_class;
        } else {
            icon_texture_path = ResourcePaths::CARD_ICON_MISSING;
            short_name = "?????";
            element = Element::None;
            limit = 0;
            card_class = Default::default();
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
        let mut label = TextStyle::new_monospace(game_io, FontName::Thick);
        label.shadow_color = TEXT_DARK_SHADOW_COLOR;
        label.color = color;

        // name
        label.bounds.set_position(NAME_OFFSET + position);
        label.draw(game_io, sprite_queue, short_name);

        // element
        let mut element_sprite = ElementSprite::new(game_io, element);
        element_sprite.set_position(ELEMENT_OFFSET + position);
        sprite_queue.draw_sprite(&element_sprite);

        if card_class != CardClass::Recipe {
            // code
            label.bounds.set_position(CODE_OFFSET + position);
            label.color = Color::WHITE;
            label.draw(game_io, sprite_queue, &self.code);
        }

        // limit
        label.font = FontName::Code;
        label.bounds.set_position(LIMIT_OFFSET + position);
        let text = format!("{limit:>2}");
        label.color = Color::from((247, 214, 99, 255));
        label.draw(game_io, sprite_queue, &text);

        label.bounds.set_position(LIM_OFFSET + position);
        label.color = Color::WHITE;
        label.draw(game_io, sprite_queue, "LM");
    }
}
