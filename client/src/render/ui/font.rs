use crate::render::{AnimationFrame, Animator};
use crate::resources::{AssetManager, LocalAssetManager, ResourcePaths};
use framework::common::GameIO;
use framework::graphics::Texture;
use framework::math::Vec2;
use std::borrow::Cow;
use std::collections::HashMap;
use std::sync::Arc;

const SPLIT_PATTERN: &str = "_U+";

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum FontName {
    Thick,
    Thin,
    ThinSmall,
    Micro,
    MenuTitle,
    Navigation,
    Context,
    Code,
    PlayerHp,
    PlayerHpOrange,
    PlayerHpGreen,
    Damage,
    Result,
    Battle,
    EntityHp,
    EntityHpRed,
    EntityHpGreen,
    DuplicateCount,
    External(Arc<str>),
}

impl FontName {
    pub fn from_name(name: &str) -> Self {
        let uppercase_name = name.to_uppercase();

        match uppercase_name.as_str() {
            "THICK" => FontName::Thick,
            "THIN" => FontName::Thin,
            "THIN_SMALL" => FontName::ThinSmall,
            "MENU_TITLE" => FontName::MenuTitle,
            "NAVIGATION" => FontName::Navigation,
            "MICRO" => FontName::Micro,
            "CONTEXT" => FontName::Context,
            "CODE" => FontName::Code,
            "PLAYER_HP" => FontName::PlayerHp,
            "PLAYER_HP_ORANGE" => FontName::PlayerHpOrange,
            "PLAYER_HP_GREEN" => FontName::PlayerHpGreen,
            "DAMAGE" => FontName::Damage,
            "RESULT" => FontName::Result,
            "BATTLE" => FontName::Battle,
            "ENTITY_HP" => FontName::EntityHp,
            "ENTITY_HP_RED" => FontName::EntityHpRed,
            "ENTITY_HP_GREEN" => FontName::EntityHpGreen,
            "DUPLICATE_COUNT" => FontName::DuplicateCount,
            _ => FontName::External(uppercase_name.into()),
        }
    }

    pub fn from_state_prefix(state_prefix: &str) -> Option<Self> {
        let font_name = match state_prefix {
            "THICK_U+" => FontName::Thick,
            "THIN_U+" => FontName::Thin,
            "THIN_SMALL_U+" => FontName::ThinSmall,
            "MICRO_U+" => FontName::Micro,
            "MENU_TITLE_U+" => FontName::MenuTitle,
            "NAVIGATION_U+" => FontName::Navigation,
            "CONTEXT_U+" => FontName::Context,
            "CODE_U+" => FontName::Code,
            "PLAYER_HP_U+" => FontName::PlayerHp,
            "PLAYER_HP_ORANGE_U+" => FontName::PlayerHpOrange,
            "PLAYER_HP_GREEN_U+" => FontName::PlayerHpGreen,
            "DAMAGE_U+" => FontName::Damage,
            "RESULT_U+" => FontName::Result,
            "BATTLE_U+" => FontName::Battle,
            "ENTITY_HP_U+" => FontName::EntityHp,
            "ENTITY_HP_RED_U+" => FontName::EntityHpRed,
            "ENTITY_HP_GREEN_U+" => FontName::EntityHpGreen,
            "DUPLICATE_COUNT_U+" => FontName::DuplicateCount,
            _ => {
                let name_end = state_prefix.rfind(SPLIT_PATTERN)?;
                let name = &state_prefix[0..name_end];

                FontName::External(name.to_uppercase().into())
            }
        };

        Some(font_name)
    }
}

pub struct GlyphAtlas {
    fonts: Vec<FontName>,
    texture: Arc<Texture>,
    map: HashMap<(Cow<'static, FontName>, Cow<'static, str>), AnimationFrame>,
}

impl GlyphAtlas {
    pub fn new_default(game_io: &GameIO, assets: &LocalAssetManager) -> Self {
        const TEXTURE_PATH: &str = ResourcePaths::FONTS;
        const ANIMATION_PATH: &str = ResourcePaths::FONTS_ANIMATION;

        let glyph_atlas = Self::new(game_io, assets, TEXTURE_PATH, ANIMATION_PATH);

        // make sure every font in the map exists in the FontName enum
        for font in &glyph_atlas.fonts {
            if let FontName::External(name) = font {
                log::warn!("{ANIMATION_PATH:?} has invalid font: {name:?}");
            }
        }

        glyph_atlas
    }

    pub fn new(
        game_io: &GameIO,
        assets: &impl AssetManager,
        texture_path: &str,
        animation_path: &str,
    ) -> Self {
        // load texture
        let texture = assets.texture(game_io, texture_path);

        let animator = Animator::load_new(assets, animation_path);
        let mut map = HashMap::new();
        let mut fonts = Vec::new();

        for (state, frames) in animator.iter_states() {
            let Some(pattern_index) = state.rfind(SPLIT_PATTERN) else {
                log::warn!("{animation_path:?} has invalid state: {state:?}");
                continue;
            };

            let split_index = pattern_index + SPLIT_PATTERN.len();
            let (font_prefix, glyph_hex) = state.split_at(split_index);

            let Some(font) = FontName::from_state_prefix(font_prefix) else {
                log::warn!("{animation_path:?} has invalid font prefix: {font_prefix:?}");
                continue;
            };

            // translate the unicode into characters for a simpler and faster lookup
            let Some(glyph_string) = Self::parse_glyph_hex(glyph_hex) else {
                log::warn!("{animation_path:?} has invalid glyph hex {glyph_hex:?} in {state:?}");
                continue;
            };

            let frame = frames.frame(0).cloned().unwrap_or_default();

            // attempt inserting frame for lowercase ascii
            if let Some(char) = glyph_string.chars().next()
                && char.is_ascii_uppercase()
                && glyph_string.is_ascii()
            {
                let lowercase_glyph_string = char.to_ascii_lowercase().to_string().into();

                map.entry((Cow::Owned(font.clone()), lowercase_glyph_string))
                    .or_insert_with(|| frame.clone());
            }

            if !fonts.contains(&font) {
                fonts.push(font.clone());
            }

            // insert frame for unmodified character
            map.insert((Cow::Owned(font), glyph_string.into()), frame);
        }

        Self {
            fonts,
            texture,
            map,
        }
    }

    pub fn texture(&self) -> &Arc<Texture> {
        &self.texture
    }

    pub fn contains_font(&self, font: &FontName) -> bool {
        self.fonts.contains(font)
    }

    fn parse_glyph_hex(glyph_hex: &str) -> Option<String> {
        glyph_hex
            .split('_')
            .map(|code_point_hex| {
                let number = u32::from_str_radix(code_point_hex, 16).ok();
                number.and_then(char::from_u32)
            })
            .collect()
    }

    pub fn character_frame<'a>(
        &'a self,
        font: &'a FontName,
        character: &'a str,
    ) -> Option<&'a AnimationFrame> {
        if let Some(frame) = self
            .map
            .get(&(Cow::Borrowed(font), Cow::Borrowed(character)))
        {
            return Some(frame);
        }

        let character = remove_diacritics(character);

        self.map
            .get(&(Cow::Borrowed(font), Cow::Borrowed(character)))
    }

    pub fn resolve_whitespace_size(&self, font: &FontName) -> Vec2 {
        self.character_frame(font, " ")
            .or_else(|| self.character_frame(font, "A"))
            .map(|frame| frame.bounds.size())
            .unwrap_or_default()
    }
}

fn remove_diacritics(character: &str) -> &str {
    match character {
        "Á" | "Ä" => "A",
        "É" => "E",
        "Í" => "I",
        "Ó" | "Ö" => "O",
        "Ú" | "Ü" => "U",

        "á" | "ä" => "a",
        "é" => "e",
        "í" => "i",
        "ó" | "ö" => "o",
        "ú" | "ü" => "u",

        c => c,
    }
}
