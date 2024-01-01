use crate::render::{AnimationFrame, Animator};
use crate::resources::{LocalAssetManager, ResourcePaths};
use framework::math::Vec2;
use std::borrow::Cow;
use std::collections::HashMap;

#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub enum FontStyle {
    Thick,
    Small,
    Tiny,
    Context,
    Wide,
    Thin,
    Gradient,
    GradientGold,
    GradientGreen,
    GradientOrange,
    GradientTall,
    Battle,
    EntityHP,
}

impl FontStyle {
    pub fn from_state_prefix(state_prefix: &str) -> Option<Self> {
        let font_style = match state_prefix {
            "THICK_U+" => FontStyle::Thick,
            "SMALL_U+" => FontStyle::Small,
            "TINY_U+" => FontStyle::Tiny,
            "CONTEXT_U+" => FontStyle::Context,
            "WIDE_U+" => FontStyle::Wide,
            "THIN_U+" => FontStyle::Thin,
            "GRADIENT_U+" => FontStyle::Gradient,
            "GRADIENT_GOLD_U+" => FontStyle::GradientGold,
            "GRADIENT_GREEN_U+" => FontStyle::GradientGreen,
            "GRADIENT_ORANGE_U+" => FontStyle::GradientOrange,
            "GRADIENT_TALL_U+" => FontStyle::GradientTall,
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
            FontStyle::Thick | FontStyle::Small | FontStyle::Tiny | FontStyle::Thin
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
