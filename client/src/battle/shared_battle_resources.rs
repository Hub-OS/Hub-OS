use crate::bindable::{AuxVariable, MathExpr};
use crate::lua_api::BattleVmManager;
use crate::packages::{PackageInfo, PackageNamespace};
use crate::render::ui::GlyphAtlas;
use crate::render::Animator;
use crate::resources::{AssetManager, Globals, ResourcePaths};
use crate::scenes::BattleEvent;
use crate::RESOLUTION_F;
use framework::math::{Rect, Vec2};
use framework::prelude::{Color, GameIO, Sprite, Texture};
use std::borrow::Cow;
use std::cell::{Cell, RefCell};
use std::collections::HashMap;
use std::sync::Arc;

use super::{BattleSimulation, StatusRegistry};

/// Resources that are shared between battle snapshots
pub struct SharedBattleResources {
    pub vm_manager: BattleVmManager,
    pub status_registry: StatusRegistry,
    pub statuses_texture: Arc<Texture>,
    pub statuses_animator: RefCell<Animator>,
    pub alert_animator: RefCell<Animator>,
    pub math_expressions:
        RefCell<HashMap<String, rollback_mlua::Result<MathExpr<f32, AuxVariable>>>>,
    pub glyph_atlases: RefCell<HashMap<(Cow<'static, str>, Cow<'static, str>), Arc<GlyphAtlas>>>,
    pub fade_color: Cell<Color>,
    pub fade_sprite: Sprite,
    pub event_sender: flume::Sender<BattleEvent>,
    pub event_receiver: flume::Receiver<BattleEvent>,
}

impl SharedBattleResources {
    pub fn new(
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        dependencies: &[(&PackageInfo, PackageNamespace)],
    ) -> Self {
        let assets = &game_io.resource::<Globals>().unwrap().assets;

        let (event_sender, event_receiver) = flume::unbounded();

        let fade_sprite_texture = assets.texture(game_io, ResourcePaths::WHITE_PIXEL);

        let fade_color = Cell::new(Color::new(0.0, 0.0, 0.0, 0.0));

        let mut fade_sprite = Sprite::new(game_io, fade_sprite_texture);
        fade_sprite.set_bounds(Rect::from_corners(Vec2::ZERO, RESOLUTION_F));

        let mut resources = Self {
            vm_manager: BattleVmManager::new(),
            status_registry: StatusRegistry::new(),
            statuses_texture: assets.texture(game_io, ResourcePaths::BATTLE_STATUSES),
            statuses_animator: RefCell::new(Animator::load_new(
                assets,
                ResourcePaths::BATTLE_STATUSES_ANIMATION,
            )),
            alert_animator: RefCell::new(
                Animator::load_new(assets, ResourcePaths::BATTLE_ALERT_ANIMATION).with_state("UI"),
            ),
            math_expressions: Default::default(),
            glyph_atlases: Default::default(),
            fade_color,
            fade_sprite,
            event_sender,
            event_receiver,
        };

        resources.init(game_io, simulation, dependencies);

        resources
    }

    fn init(
        &mut self,
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        dependencies: &[(&PackageInfo, PackageNamespace)],
    ) {
        BattleVmManager::init(game_io, self, simulation, dependencies);
        self.status_registry
            .init(game_io, &self.vm_manager, dependencies);
    }

    pub fn parse_math_expr(
        &self,
        source: String,
    ) -> rollback_mlua::Result<MathExpr<f32, AuxVariable>> {
        let mut expressions = self.math_expressions.borrow_mut();

        expressions
            .entry(source)
            .or_insert_with_key(|source| {
                MathExpr::parse(source)
                    .map_err(|err| rollback_mlua::Error::RuntimeError(err.to_string()))
            })
            .clone()
    }
}
