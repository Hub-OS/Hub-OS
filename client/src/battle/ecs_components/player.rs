use crate::battle::*;
use crate::bindable::*;
use crate::packages::PackageNamespace;
use crate::render::*;
use crate::resources::*;
use crate::saves::Card;
use framework::prelude::*;

#[derive(Clone)]
pub struct Player {
    pub index: usize,
    pub local: bool,
    pub cards: Vec<Card>,
    pub card_use_requested: bool,
    pub charge: u8,
    pub attack: u8,
    pub speed: u8,
    pub card_view_size: u8,
    pub modded_hp: i32,
    pub charging_time: FrameTime,
    pub max_charging_time: FrameTime,
    pub charge_sprite_index: TreeIndex,
    pub charge_animator: Animator,
    pub charged_color: Color,
    pub slide_when_moving: bool,
    pub forms: Vec<PlayerForm>,
    pub active_form: Option<usize>,
    pub normal_attack_callback: BattleCallback<(), Option<GenerationalIndex>>,
    pub charged_attack_callback: BattleCallback<(), Option<GenerationalIndex>>,
    pub special_attack_callback: BattleCallback<(), Option<GenerationalIndex>>,
}

impl Player {
    pub const IDLE_STATE: &str = "PLAYER_IDLE";
    pub const CHARGE_DELAY: FrameTime = 10;

    pub const MOVE_FRAMES: [DerivedFrame; 7] = [
        DerivedFrame::new(0, 1),
        DerivedFrame::new(1, 1),
        DerivedFrame::new(2, 1),
        DerivedFrame::new(3, 1),
        DerivedFrame::new(2, 1),
        DerivedFrame::new(1, 1),
        DerivedFrame::new(0, 1),
    ];

    pub const HIT_FRAMES: [DerivedFrame; 2] = [DerivedFrame::new(0, 15), DerivedFrame::new(1, 7)];

    pub fn new(
        game_io: &GameIO,
        index: usize,
        local: bool,
        charge_sprite_index: TreeIndex,
        cards: Vec<Card>,
    ) -> Self {
        let assets = &game_io.resource::<Globals>().unwrap().assets;

        Self {
            index,
            local,
            cards,
            card_use_requested: false,
            charge: 1,
            attack: 1,
            speed: 1,
            card_view_size: 5,
            modded_hp: 0,
            charging_time: 0,
            max_charging_time: 100,
            charge_sprite_index,
            charge_animator: Animator::load_new(assets, ResourcePaths::BATTLE_CHARGE_ANIMATION),
            charged_color: Color::MAGENTA,
            slide_when_moving: false,
            forms: Vec::new(),
            active_form: None,
            normal_attack_callback: BattleCallback::stub(None),
            charged_attack_callback: BattleCallback::stub(None),
            special_attack_callback: BattleCallback::stub(None),
        }
    }

    pub fn available_forms(&self) -> impl Iterator<Item = (usize, &PlayerForm)> {
        self.forms
            .iter()
            .enumerate()
            .filter(|(_, form)| !form.activated)
    }

    pub fn namespace(&self) -> PackageNamespace {
        if self.local {
            PackageNamespace::Local
        } else {
            PackageNamespace::Remote(self.index)
        }
    }
}
