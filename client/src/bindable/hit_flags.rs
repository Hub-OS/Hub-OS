pub type HitFlags = u64;

// simulating an enum here, could use a better solution
#[allow(non_snake_case)]
pub mod HitFlag {
    use crate::battle::StatusRegistry;

    use super::HitFlags;
    use framework::prelude::Vec2;

    pub const NONE: HitFlags = 0;
    pub const RETAIN_INTANGIBLE: HitFlags = 1;
    pub const NO_COUNTER: HitFlags = 1 << 2;
    pub const DRAG: HitFlags = 1 << 3;
    pub const IMPACT: HitFlags = 1 << 4;
    pub const FLINCH: HitFlags = 1 << 5;
    pub const FLASH: HitFlags = 1 << 6;
    pub const SHAKE: HitFlags = 1 << 7;
    pub const PIERCE_INVIS: HitFlags = 1 << 8;
    pub const PIERCE_GUARD: HitFlags = 1 << 9; // NOTE: this is what we refer to as "true breaking"
    pub const PIERCE_GROUND: HitFlags = 1 << 10;

    pub const FREEZE: HitFlags = 1 << 11;
    pub const PARALYZE: HitFlags = 1 << 12;
    pub const ROOT: HitFlags = 1 << 13;
    pub const BLIND: HitFlags = 1 << 14;
    pub const CONFUSE: HitFlags = 1 << 15;

    pub const BUILT_IN: [HitFlags; 15] = [
        RETAIN_INTANGIBLE,
        FREEZE,
        PIERCE_INVIS,
        FLINCH,
        SHAKE,
        PARALYZE,
        FLASH,
        PIERCE_GUARD,
        IMPACT,
        DRAG,
        NO_COUNTER,
        ROOT,
        BLIND,
        CONFUSE,
        PIERCE_GROUND,
    ];

    pub fn from_str(status_registry: &StatusRegistry, s: &str) -> HitFlags {
        match s {
            "RetainIntangible" => RETAIN_INTANGIBLE,
            "Freeze" => FREEZE,
            "PierceInvis" => PIERCE_INVIS,
            "Flinch" => FLINCH,
            "Shake" => SHAKE,
            "Paralyze" => PARALYZE,
            "Flash" => FLASH,
            "PierceGuard" => PIERCE_GUARD,
            "Impact" => IMPACT,
            "Drag" => DRAG,
            "NoCounter" => NO_COUNTER,
            "Root" => ROOT,
            "Blind" => BLIND,
            "Confuse" => CONFUSE,
            "PierceGround" => PIERCE_GROUND,
            _ => status_registry.resolve_flag(s).unwrap_or(NONE),
        }
    }

    pub const BUILT_IN_STATUSES: [HitFlags; 5] = [FREEZE, PARALYZE, ROOT, BLIND, CONFUSE];

    pub fn status_animation_state(flag: HitFlags, height: f32) -> &'static str {
        match flag {
            CONFUSE => "CONFUSE",
            BLIND => "BLIND",
            FREEZE => {
                if height <= 48.0 {
                    "FREEZE_SMALL"
                } else if height <= 75.0 {
                    "FREEZE_MEDIUM"
                } else {
                    "FREEZE_LARGE"
                }
            }
            _ => "",
        }
    }

    pub fn status_sprite_position(flag: HitFlags, height: f32) -> Vec2 {
        match flag {
            CONFUSE => Vec2::new(0.0, -height),
            BLIND => Vec2::new(0.0, -height),
            FREEZE => Vec2::new(0.0, -height * 0.5),
            _ => Vec2::ZERO,
        }
    }
}
