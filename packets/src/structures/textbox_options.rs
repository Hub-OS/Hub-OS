use serde::{Deserialize, Serialize};
use std::{borrow::Cow, ops::Deref};

#[derive(Debug, Default, Clone, PartialEq, Eq, Hash, Deserialize, Serialize)]
pub struct TextureAnimPathPair<'a> {
    pub texture: Cow<'a, str>,
    pub animation: Cow<'a, str>,
}

impl TextureAnimPathPair<'_> {
    pub fn own(self) -> TextureAnimPathPair<'static> {
        TextureAnimPathPair {
            texture: Cow::Owned(self.texture.into_owned()),
            animation: Cow::Owned(self.animation.into_owned()),
        }
    }

    pub fn dependencies(&self) -> impl Iterator<Item = &str> {
        [self.texture.deref(), self.animation.deref()].into_iter()
    }
}

#[derive(Debug, Clone, PartialEq, Deserialize, Serialize)]
pub struct TextStyleBlueprint {
    pub custom_atlas: Option<TextureAnimPathPair<'static>>,
    pub font_name: String,
    pub monospace: bool,
    pub min_glyph_width: f32,
    pub letter_spacing: f32,
    pub line_spacing: f32,
    pub scale_x: f32,
    pub scale_y: f32,
    pub color: (u8, u8, u8),
    pub shadow_color: (u8, u8, u8, u8),
}

impl Default for TextStyleBlueprint {
    fn default() -> Self {
        Self {
            custom_atlas: None,
            font_name: Default::default(),
            monospace: false,
            min_glyph_width: 6.0,
            letter_spacing: 1.0,
            line_spacing: 3.0,
            scale_x: 1.0,
            scale_y: 1.0,
            color: (0, 0, 0),
            shadow_color: (0, 0, 0, 0),
        }
    }
}

impl TextStyleBlueprint {
    pub fn dependencies(&self) -> impl Iterator<Item = &str> {
        self.custom_atlas.iter().flat_map(|v| v.dependencies())
    }
}

#[derive(Debug, Default, Clone, PartialEq, Deserialize, Serialize)]
pub struct TextboxOptions {
    pub mug: Option<TextureAnimPathPair<'static>>,
    pub text_style: Option<TextStyleBlueprint>,
}

impl TextboxOptions {
    pub fn dependencies(&self) -> impl Iterator<Item = &str> {
        let mug_dependencies = self.mug.iter().flat_map(|v| v.dependencies());
        let text_style_dependencies = self.text_style.iter().flat_map(|v| v.dependencies());

        mug_dependencies.chain(text_style_dependencies)
    }
}
