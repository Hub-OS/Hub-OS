use crate::overworld::OverworldPlayerData;
use crate::render::ui::{draw_clock, draw_date, ScrollTracker, UiInputTracker};
use crate::render::*;
use crate::resources::*;
use crate::scenes::*;
use framework::prelude::*;

use super::{FontStyle, TextStyle};

#[derive(Clone, Copy)]
pub enum SceneOption {
    Servers,
    Folders,
    Library,
    Character,
    BattleSelect,
    Config,
}

#[derive(Clone, Copy, PartialEq, Eq)]
enum OpenState {
    Open,
    Closed,
    Opening,
    Closing,
}

impl OpenState {
    fn is_animated(self) -> bool {
        match self {
            OpenState::Open => false,
            OpenState::Closed => false,
            OpenState::Closing => true,
            OpenState::Opening => true,
        }
    }
}

impl SceneOption {
    fn state(&self) -> &'static str {
        match self {
            SceneOption::Servers => "SERVERS_LABEL",
            SceneOption::Folders => "FOLDERS_LABEL",
            SceneOption::Library => "LIBRARY_LABEL",
            SceneOption::Character => "CHARACTER_LABEL",
            SceneOption::BattleSelect => "BATTLE_SELECT_LABEL",
            SceneOption::Config => "CONFIG_LABEL",
        }
    }
}

struct NavigationItem {
    sprite: Sprite,
    target_scene: SceneOption,
}

pub struct NavigationMenu {
    open_state: OpenState,
    overlay: bool,
    animation_time: FrameTime,
    scroll_tracker: ScrollTracker,
    scroll_top: f32,
    hp_point: Vec2,
    money_point: Vec2,
    hp_text: String,
    money_text: String,
    animator: Animator,
    top_bar_sprite: Sprite,
    info_sprite: Sprite,
    fade_sprite: Sprite,
    items: Vec<NavigationItem>,
    ui_input_tracker: UiInputTracker,
}

impl NavigationMenu {
    pub fn new(game_io: &GameIO<Globals>, items: Vec<SceneOption>) -> NavigationMenu {
        let globals = game_io.globals();
        let assets = &globals.assets;

        // menu assets
        let menu_sprite = assets.new_sprite(game_io, ResourcePaths::MAIN_MENU_PARTS);
        let mut animator = Animator::load_new(assets, ResourcePaths::MAIN_MENU_PARTS_ANIMATION);

        // top bar
        let mut top_bar_sprite = menu_sprite.clone();
        animator.set_state("TOP_BAR");
        animator.apply(&mut top_bar_sprite);

        // scroll tracker
        let mut scroll_tracker = ScrollTracker::new(game_io, 6).with_wrap(true);
        scroll_tracker.set_total_items(items.len());

        // menu items
        let items = items
            .into_iter()
            .enumerate()
            .map(|(i, target_scene)| {
                let mut sprite = menu_sprite.clone();
                animator.set_state(target_scene.state());

                if i == 0 {
                    // use selected sprite
                    animator.set_frame(1);
                }

                animator.apply(&mut sprite);

                NavigationItem {
                    sprite,
                    target_scene,
                }
            })
            .collect();

        // info sprite
        let mut info_sprite = menu_sprite.clone();
        animator.set_state("INFO");
        animator.apply(&mut info_sprite);

        let hp_point = animator.point("HP").unwrap_or_default() - animator.origin();
        let money_point = animator.point("MONEY").unwrap_or_default() - animator.origin();

        // backing fade sprite
        let mut fade_sprite = assets.new_sprite(game_io, ResourcePaths::WHITE_PIXEL);
        fade_sprite.set_color(Color::TRANSPARENT);
        fade_sprite.set_bounds(Rect::from_corners(Vec2::ZERO, RESOLUTION_F));

        NavigationMenu {
            open_state: OpenState::Open,
            overlay: false,
            animation_time: 0,
            scroll_tracker,
            scroll_top: 0.0,
            hp_point,
            money_point,
            hp_text: String::new(),
            money_text: String::new(),
            animator,
            top_bar_sprite,
            info_sprite,
            fade_sprite,
            ui_input_tracker: UiInputTracker::new(),
            items,
        }
    }

    pub fn as_overlay(mut self, closeable: bool) -> Self {
        self.overlay = closeable;
        self.open_state = if closeable {
            OpenState::Closed
        } else {
            OpenState::Open
        };
        self
    }

    pub fn is_open(&self) -> bool {
        self.open_state != OpenState::Closed
    }

    pub fn open(&mut self) {
        self.open_state = OpenState::Opening;
        self.animation_time = 0;

        let prev_index = self.scroll_tracker.selected_index();
        self.scroll_tracker.set_selected_index(0);
        self.update_item_sprites(prev_index);
    }

    pub fn close(&mut self) {
        self.open_state = OpenState::Closing;
        self.animation_time = 0;
    }

    pub fn update_info(&mut self, player_data: &OverworldPlayerData) {
        self.hp_text = format!("{:>4}/{:>4}", player_data.health, player_data.max_health);
        self.money_text = format!("{:>8}$", player_data.money);
    }

    pub fn update(&mut self, game_io: &mut GameIO<Globals>) -> NextScene<Globals> {
        let mut next_scene = NextScene::None;

        self.ui_input_tracker.update(game_io);

        if self.open_state == OpenState::Closed {
            return next_scene;
        }

        // systems
        system_scroll(self);
        system_transition(self);

        // input based updates after this
        if self.open_state.is_animated() || game_io.is_in_transition() {
            return next_scene;
        }

        // handle close
        let input_util = InputUtil::new(game_io);
        let requesting_close =
            input_util.was_just_pressed(Input::Pause) || input_util.was_just_pressed(Input::Cancel);

        if self.overlay && requesting_close {
            let globals = game_io.globals();
            globals.audio.play_sound(&globals.menu_close_sfx);
            self.close();
        }

        // updating selection
        let prev_index = self.scroll_tracker.selected_index();

        self.scroll_tracker
            .handle_vertical_input(&self.ui_input_tracker);

        if prev_index != self.scroll_tracker.selected_index() {
            let globals = game_io.globals();
            globals.audio.play_sound(&globals.cursor_move_sfx);
            self.update_item_sprites(prev_index);
        }

        if self.ui_input_tracker.is_active(Input::Confirm) {
            next_scene = self.select_item(game_io);
        }

        next_scene
    }

    fn update_item_sprites(&mut self, prev_index: usize) {
        let prev_item = &mut self.items[prev_index];
        self.animator.set_state(prev_item.target_scene.state());
        self.animator.set_frame(0);
        self.animator.apply(&mut prev_item.sprite);

        let item = &mut self.items[self.scroll_tracker.selected_index()];
        self.animator.set_state(item.target_scene.state());
        self.animator.set_frame(1);
        self.animator.apply(&mut item.sprite);
    }

    fn select_item(&mut self, game_io: &mut GameIO<Globals>) -> NextScene<Globals> {
        use crate::transitions::*;

        let selection = self.scroll_tracker.selected_index();

        let scene: Option<Box<dyn Scene<Globals>>> = match self.items[selection].target_scene {
            SceneOption::Servers => Some(ServerListScene::new(game_io)),
            SceneOption::Folders => Some(FolderListScene::new(game_io)),
            SceneOption::Library => Some(LibraryScene::new(game_io)),
            SceneOption::BattleSelect => Some(BattleSelectScene::new(game_io)),
            SceneOption::Config => Some(ConfigScene::new(game_io)),
            _ => None,
        };

        let globals = game_io.globals();

        if let Some(scene) = scene {
            globals.audio.play_sound(&globals.cursor_select_sfx);

            let transition = PushTransition::new(
                game_io,
                game_io.globals().default_sampler.clone(),
                Direction::Right,
                DEFAULT_PUSH_DURATION,
            );

            NextScene::Push {
                scene,
                transition: Some(Box::new(transition)),
            }
        } else {
            globals.audio.play_sound(&globals.cursor_error_sfx);
            log::warn!("Not implemented");
            NextScene::None
        }
    }

    pub fn draw(&mut self, game_io: &GameIO<Globals>, sprite_queue: &mut SpriteColorQueue) {
        if self.open_state == OpenState::Closed {
            return;
        }

        // draw backing fade
        if self.overlay {
            sprite_queue.draw_sprite(&self.fade_sprite);
        }

        // draw items
        for item in &self.items {
            sprite_queue.draw_sprite(&item.sprite);
        }

        // draw top bar
        sprite_queue.draw_sprite(&self.top_bar_sprite);

        // draw info
        if self.overlay {
            sprite_queue.draw_sprite(&self.info_sprite);
        }

        if !self.open_state.is_animated() {
            draw_date(game_io, sprite_queue);
            draw_clock(game_io, sprite_queue);

            if self.overlay {
                let mut text_style = TextStyle::new_monospace(game_io, FontStyle::Thin);

                text_style.bounds.set_position(self.hp_point);
                text_style.draw(game_io, sprite_queue, &self.hp_text);

                text_style.bounds.set_position(self.money_point);
                text_style.draw(game_io, sprite_queue, &self.money_text);
            }
        }
    }
}

fn system_scroll(menu: &mut NavigationMenu) {
    const OFFSET: Vec2 = Vec2::new(RESOLUTION_F.x - 80.0, 28.0);
    const ITEM_OFFSET: f32 = 21.0;

    let target_scroll_top = menu.scroll_tracker.top_index() as f32;

    menu.scroll_top = menu.scroll_top + (target_scroll_top - menu.scroll_top) * 0.2;

    for (i, item) in menu.items.iter_mut().enumerate() {
        let mut position = OFFSET;
        position.y += i as f32 * ITEM_OFFSET;
        position.y -= menu.scroll_top * ITEM_OFFSET;

        item.sprite.set_position(position);
    }
}

fn system_transition(menu: &mut NavigationMenu) {
    if !menu.open_state.is_animated() {
        return;
    }

    const ANIMATION_TIME: FrameTime = 8;

    let animation_progress = match menu.open_state {
        OpenState::Opening => menu.animation_time as f32 / ANIMATION_TIME as f32,
        OpenState::Closing => 1.0 - menu.animation_time as f32 / ANIMATION_TIME as f32,
        _ => unreachable!(),
    };

    menu.animation_time += 1;

    if menu.animation_time > ANIMATION_TIME {
        menu.open_state = match menu.open_state {
            OpenState::Opening => OpenState::Open,
            OpenState::Closing => OpenState::Closed,
            _ => unreachable!(),
        };
    }

    // animate top bar
    let top_bar_height = menu.top_bar_sprite.bounds().height;
    let position = Vec2::new(0.0, -top_bar_height * (1.0 - animation_progress));
    menu.top_bar_sprite.set_position(position);

    // animate menu items
    for item in &mut menu.items {
        let mut position = item.sprite.position();
        position.x = (position.x - RESOLUTION_F.x) * animation_progress + RESOLUTION_F.x;

        item.sprite.set_position(position);
    }

    // animate info
    let info_origin = menu.info_sprite.origin();
    let info_width = menu.info_sprite.bounds().width;
    let position = Vec2::new(
        (-info_width + info_origin.x) * (1.0 - animation_progress),
        0.0,
    );

    menu.info_sprite.set_position(position);

    // animate fade
    let opacity = animation_progress * 0.4;
    let fade_color = Color::BLACK.multiply_alpha(opacity);
    menu.fade_sprite.set_color(fade_color);
}
