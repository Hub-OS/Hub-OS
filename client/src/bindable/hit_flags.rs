pub type HitFlags = i64;

// simulating an enum here, could use a better solution
#[allow(non_snake_case)]
pub mod HitFlag {
    use crate::battle::StatusRegistry;

    use super::HitFlags;
    use framework::prelude::Vec2;

    pub const ALL: HitFlags = !0;
    pub const NONE: HitFlags = 0;
    pub const RETAIN_INTANGIBLE: HitFlags = 1;
    pub const NO_COUNTER: HitFlags = 1 << 1;
    pub const DRAG: HitFlags = 1 << 2;
    pub const DRAIN: HitFlags = 1 << 3;
    pub const FLINCH: HitFlags = 1 << 4;
    pub const PIERCE_INVIS: HitFlags = 1 << 5;
    pub const PIERCE_GUARD: HitFlags = 1 << 6; // NOTE: this is what we refer to as "true breaking"
    pub const PIERCE_GROUND: HitFlags = 1 << 7;

    pub const FLASH: HitFlags = 1 << 8;
    pub const PARALYZE: HitFlags = 1 << 9;
    pub const BLIND: HitFlags = 1 << 10;
    pub const NEXT_SHIFT: HitFlags = 11;

    pub const BAKED: [HitFlags; 8] = [
        RETAIN_INTANGIBLE,
        NO_COUNTER,
        DRAG,
        DRAIN,
        FLINCH,
        PIERCE_INVIS,
        PIERCE_GUARD,
        PIERCE_GROUND,
    ];

    pub const KEEP_IN_FREEZE: HitFlags = BLIND;

    pub fn from_str(status_registry: &StatusRegistry, s: &str) -> HitFlags {
        match s {
            "None" => NONE,
            "RetainIntangible" => RETAIN_INTANGIBLE,
            "NoCounter" => NO_COUNTER,
            "Drag" => DRAG,
            "Drain" => DRAIN,
            "Flinch" => FLINCH,
            "Flash" => FLASH,
            "PierceInvis" => PIERCE_INVIS,
            "PierceGuard" => PIERCE_GUARD,
            "PierceGround" => PIERCE_GROUND,
            "Paralyze" => PARALYZE,
            "Blind" => BLIND,
            _ => status_registry.resolve_flag(s).unwrap_or(NONE),
        }
    }

    pub fn status_animation_state(flag: HitFlags) -> &'static str {
        match flag {
            BLIND => "BLIND",
            _ => "",
        }
    }

    pub fn status_sprite_position(flag: HitFlags, height: f32) -> Vec2 {
        match flag {
            BLIND => Vec2::new(0.0, -height),
            _ => Vec2::ZERO,
        }
    }
}
