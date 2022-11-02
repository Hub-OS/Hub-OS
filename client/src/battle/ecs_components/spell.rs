use crate::bindable::*;

#[derive(Default, Clone)]
pub struct Spell {
    pub requested_highlight: TileHighlight,
    pub hit_props: HitProperties,
}
