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
    pub fn from_cow(mut name: Cow<str>) -> Self {
        if !name.chars().all(|c| c.is_uppercase()) {
            name = match name {
                Cow::Borrowed(s) => s.to_uppercase().into(),
                Cow::Owned(mut s) => {
                    s.make_ascii_lowercase();
                    s.into()
                }
            }
        }

        match &*name {
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
            _ => FontName::External(name.into()),
        }
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
            let Some(split_index) = state.find(SPLIT_PATTERN) else {
                log::warn!("{animation_path:?} has invalid state: {state:?}");
                continue;
            };

            let (font_prefix, split_str) = state.split_at(split_index);
            let hex_str = &split_str[SPLIT_PATTERN.len()..];

            let font = FontName::from_cow(Cow::Borrowed(font_prefix));

            // translate the unicode into characters for a simpler and faster lookup
            let Some(grapheme) = Self::parse_glyph_hex(hex_str) else {
                log::warn!("{animation_path:?} has invalid glyph hex {hex_str:?} in {state:?}");
                continue;
            };

            let frame = frames.frame(0).cloned().unwrap_or_default();

            // attempt inserting frame for lowercase ascii
            if let Some(char) = grapheme.chars().next()
                && char.is_uppercase()
            {
                let lowercase_glyph_string = grapheme.to_lowercase().into();

                map.entry((Cow::Owned(font.clone()), lowercase_glyph_string))
                    .or_insert_with(|| frame.clone());
            }

            if !fonts.contains(&font) {
                fonts.push(font.clone());
            }

            // insert frame for unmodified character
            map.insert((Cow::Owned(font), grapheme.into()), frame);
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
        "Á" | "Ä" | "Â" | "Ã" => "A",
        "É" | "Ê" | "Ẽ" => "E",
        "Í" | "Î" => "I",
        "Ó" | "Ö" | "Ô" | "Õ" => "O",
        "Ú" | "Ü" | "Û" => "U",

        "á" | "ä" | "â" | "ã" | "à" => "a",
        "é" | "ê" | "ẽ" => "e",
        "í" | "î" => "i",
        "ó" | "ö" | "ô" | "õ" => "o",
        "ú" | "ü" | "û" => "u",

        "Ç" => "C",
        "ç" => "c",

        c => c,
    }
}
