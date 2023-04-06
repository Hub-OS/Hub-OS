use num_derive::FromPrimitive;

#[repr(u8)]
#[derive(Debug, Default, Clone, Copy, PartialEq, Eq, FromPrimitive)]
pub enum SpriteShaderEffect {
    #[default]
    Default,
    Greyscale,
    Pixelate,
    Palette,
    PixelatePalette,
}

impl SpriteShaderEffect {
    pub fn requires_palette(self) -> bool {
        matches!(
            self,
            SpriteShaderEffect::Palette | SpriteShaderEffect::PixelatePalette
        )
    }
}
