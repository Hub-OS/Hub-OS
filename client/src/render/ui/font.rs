use crate::render::{AnimationFrame, Animator};
use crate::resources::{LocalAssetManager, ResourcePaths};
use framework::math::Vec2;
use std::borrow::Cow;
use std::collections::HashMap;

#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub enum FontStyle {
    Thick,
    Thin,
    ThinSmall,
    Micro,
    Context,
    Wide,
    PlayerHp,
    PlayerHpOrange,
    PlayerHpGreen,
    Damage,
    Result,
    Battle,
    EntityHP,
}

impl FontStyle {
    pub fn from_name(name: &str) -> Option<Self> {
        let font_style = match name.to_uppercase().as_str() {
            "THICK" => FontStyle::Thick,
            "THIN" => FontStyle::Thin,
            "THIN_SMALL" => FontStyle::ThinSmall,
            "MICRO" => FontStyle::Micro,
            "CONTEXT" => FontStyle::Context,
            "WIDE" => FontStyle::Wide,
            "PLAYER_HP" => FontStyle::PlayerHp,
            "PLAYER_HP_ORANGE" => FontStyle::PlayerHpOrange,
            "PLAYER_HP_GREEN" => FontStyle::PlayerHpGreen,
            "DAMAGE" => FontStyle::Damage,
            "RESULT" => FontStyle::Result,
            "BATTLE" => FontStyle::Battle,
            "ENTITY_HP" => FontStyle::EntityHP,
            _ => {
                return None;
            }
        };

        Some(font_style)
    }

    pub fn from_state_prefix(state_prefix: &str) -> Option<Self> {
        let font_style = match state_prefix {
            "THICK_U+" => FontStyle::Thick,
            "THIN_U+" => FontStyle::Thin,
            "THIN_SMALL_U+" => FontStyle::ThinSmall,
            "MICRO_U+" => FontStyle::Micro,
            "CONTEXT_U+" => FontStyle::Context,
            "WIDE_U+" => FontStyle::Wide,
            "PLAYER_HP_U+" => FontStyle::PlayerHp,
            "PLAYER_HP_ORANGE_U+" => FontStyle::PlayerHpOrange,
            "PLAYER_HP_GREEN_U+" => FontStyle::PlayerHpGreen,
            "DAMAGE_U+" => FontStyle::Damage,
            "RESULT_U+" => FontStyle::Result,
            "BATTLE_U+" => FontStyle::Battle,
            "ENTITY_HP_U+" => FontStyle::EntityHP,
            _ => {
                return None;
            }
        };

        Some(font_style)
    }

    pub fn has_lower_case(&self) -> bool {
        matches!(
            self,
            FontStyle::Thick | FontStyle::Thin | FontStyle::ThinSmall | FontStyle::Micro
        )
    }
}

pub struct GlyphMap {
    map: HashMap<(FontStyle, Cow<'static, str>), AnimationFrame>,
}

impl GlyphMap {
    pub fn new(assets: &LocalAssetManager) -> Self {
        // translate + store the unicode as characters for a faster lookup
        const FONT_PATH: &str = ResourcePaths::FONTS_ANIMATION;

        let animator = Animator::load_new(assets, FONT_PATH);
        let mut map = HashMap::new();

        const SPLIT_PATTERN: &str = "_U+";

        for (state, frames) in animator.iter_states() {
            let Some(pattern_index) = state.rfind(SPLIT_PATTERN) else {
                log::warn!("{FONT_PATH:?} has invalid state: {state:?}");
                continue;
            };

            let split_index = pattern_index + SPLIT_PATTERN.len();
            let (font_prefix, glyph_hex) = state.split_at(split_index);

            let Some(font_style) = FontStyle::from_state_prefix(font_prefix) else {
                log::warn!("{FONT_PATH:?} has invalid font prefix: {font_prefix:?}");
                continue;
            };

            let Some(glyph_string) = Self::parse_glyph_hex(glyph_hex) else {
                log::warn!("{FONT_PATH:?} has invalid glyph hex {glyph_hex:?} in {state:?}");
                continue;
            };

            let frame = frames.frame(0).cloned().unwrap_or_default();

            map.insert((font_style, glyph_string.into()), frame);
        }

        Self { map }
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
        font_style: FontStyle,
        character: &'a str,
    ) -> Option<&AnimationFrame> {
        self.map.get(&(font_style, Cow::Borrowed(character)))
    }

    pub fn resolve_whitespace_size(&self, font_style: FontStyle) -> Vec2 {
        self.character_frame(font_style, " ")
            .or_else(|| self.character_frame(font_style, "A"))
            .map(|frame| frame.bounds.size())
            .unwrap_or_default()
    }
}
