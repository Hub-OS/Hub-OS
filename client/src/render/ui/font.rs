#[derive(Clone, Copy)]
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
    pub fn state_prefix(&self) -> &'static str {
        match self {
            FontStyle::Thick => "THICK_U",
            FontStyle::Small => "SMALL_U",
            FontStyle::Tiny => "TINY_U",
            FontStyle::Context => "CONTEXT_U",
            FontStyle::Wide => "WIDE_U",
            FontStyle::Thin => "THIN_U",
            FontStyle::Gradient => "GRADIENT_U",
            FontStyle::GradientGold => "GRADIENT_GOLD_U",
            FontStyle::GradientGreen => "GRADIENT_GREEN_U",
            FontStyle::GradientOrange => "GRADIENT_ORANGE_U",
            FontStyle::GradientTall => "GRADIENT_TALL_U",
            FontStyle::Battle => "BATTLE_U",
            FontStyle::EntityHP => "ENTITY_HP_U",
        }
    }

    pub fn has_lower_case(&self) -> bool {
        matches!(
            self,
            FontStyle::Thick | FontStyle::Small | FontStyle::Tiny | FontStyle::Thin
        )
    }
}
