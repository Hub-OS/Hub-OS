use num_derive::FromPrimitive;

#[repr(u8)]
#[derive(Default, Clone, Copy, PartialEq, Eq, FromPrimitive)]
pub enum SpriteShaderEffect {
    #[default]
    Default,
    Greyscale,
    Palette,
}
