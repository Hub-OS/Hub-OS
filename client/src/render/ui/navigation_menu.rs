use crate::render::ui::{draw_clock, draw_date, ScrollTracker, UiInputTracker};
use crate::render::*;
use crate::resources::*;
use crate::scenes::*;
use framework::prelude::*;

#[derive(Clone, Copy)]
pub enum SceneOption {
    Servers,
    Folders,
    Library,
    Character,
    BattleSelect,
    Config,
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
    scroll_tracker: ScrollTracker,
    scroll_top: f32,
    animator: Animator,
    top_bar_sprite: Sprite,
    items: Vec<NavigationItem>,
    ui_input_tracker: UiInputTracker,
}

impl NavigationMenu {
    pub fn new(game_io: &mut GameIO<Globals>, items: Vec<SceneOption>) -> NavigationMenu {
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
        let mut scroll_tracker = ScrollTracker::new(game_io, 6);
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

        NavigationMenu {
            scroll_tracker,
            scroll_top: 0.0,
            animator,
            top_bar_sprite,
            ui_input_tracker: UiInputTracker::new(),
            items,
        }
    }

    pub fn update(&mut self, game_io: &mut GameIO<Globals>) -> NextScene<Globals> {
        let mut next_scene = NextScene::None;

        self.ui_input_tracker.update(game_io);

        // input based update
        if !game_io.is_in_transition() {
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
        }

        // systems
        system_scroll(self);

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
        // draw items
        for item in &self.items {
            sprite_queue.draw_sprite(&item.sprite);
        }

        // draw top bar
        sprite_queue.draw_sprite(&self.top_bar_sprite);
        draw_date(game_io, sprite_queue);
        draw_clock(game_io, sprite_queue);
    }
}

fn system_scroll(scene: &mut NavigationMenu) {
    const OFFSET: Vec2 = Vec2::new(RESOLUTION_F.x - 80.0, 28.0);
    const ITEM_OFFSET: f32 = 21.0;

    let target_scroll_top = scene.scroll_tracker.top_index() as f32;

    scene.scroll_top = scene.scroll_top + (target_scroll_top - scene.scroll_top) * 0.2;

    for (i, item) in scene.items.iter_mut().enumerate() {
        let mut position = OFFSET;
        position.y += i as f32 * ITEM_OFFSET;
        position.y -= scene.scroll_top * ITEM_OFFSET;

        item.sprite.set_position(position);
    }
}
