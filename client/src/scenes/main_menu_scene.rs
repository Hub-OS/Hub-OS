use crate::bindable::SpriteColorMode;
use crate::packages::PackageNamespace;
use crate::render::ui::{FontName, NavigationMenu, SceneOption, TextStyle, TextboxQuestion};
use crate::render::*;
use crate::resources::*;
use crate::saves::GlobalSave;
use framework::prelude::*;
use ui::{Textbox, TextboxMessage};

enum Event {
    Quit,
}

struct CharacterData {
    loaded: bool,
    scrolling_text: String,
    scrolling_text_wrap: f32,
    sprite: Sprite,
    shadow_sprite: Sprite,
}

impl CharacterData {
    fn init_selected_player(game_io: &mut GameIO) {
        let globals = Globals::from_resources_mut(game_io);

        let player_packages = &globals.player_packages;
        let player_package = player_packages.package(
            PackageNamespace::Local,
            &globals.global_save.selected_character,
        );

        if player_package.is_some() {
            return;
        }

        let Some(package_id) = player_packages.package_ids(PackageNamespace::Local).next() else {
            return;
        };

        let global_save = &mut globals.global_save;
        global_save.selected_character = package_id.clone();
        global_save.selected_character_time = GlobalSave::current_time();
        global_save.save();
    }

    fn navigation_options(&self) -> Vec<SceneOption> {
        if self.loaded {
            vec![
                SceneOption::Servers,
                SceneOption::Decks,
                SceneOption::Library,
                SceneOption::Character,
                SceneOption::BattleSelect,
                SceneOption::Config,
            ]
        } else {
            vec![
                SceneOption::Decks,
                SceneOption::Library,
                SceneOption::Config,
            ]
        }
    }

    fn load(game_io: &mut GameIO, text_style: &TextStyle) -> Self {
        Self::init_selected_player(game_io);

        let globals = Globals::from_resources(game_io);
        let player_id = &globals.global_save.selected_character;
        let assets = &globals.assets;

        let player_package = globals
            .player_packages
            .package(PackageNamespace::Local, player_id);

        // sprite
        let mut sprite = if let Some(package) = player_package {
            assets.new_sprite(game_io, &package.preview_texture_path)
        } else {
            assets.new_sprite(game_io, ResourcePaths::PIXEL)
        };

        sprite.set_position(Vec2::new(0.0, RESOLUTION_F.y - sprite.size().y));

        let mut shadow_sprite = sprite.clone();

        shadow_sprite.set_origin(Vec2::new(20.0, 0.0));
        shadow_sprite.set_color(Color::new(0.0, 0.0, 0.0, 0.3));

        // text
        let part_text = player_package
            .map(|package| &*package.name)
            .unwrap_or("Missing Navi")
            .to_uppercase()
            + "   ";

        let part_width = text_style.measure(&part_text).size.x + text_style.letter_spacing;
        let part_repeats = (RESOLUTION_F.x / part_width).ceil() as usize + 1;

        let scrolling_text = part_text.repeat(part_repeats);

        Self {
            loaded: player_package.is_some(),
            scrolling_text,
            scrolling_text_wrap: part_width,
            sprite,
            shadow_sprite,
        }
    }
}

pub struct MainMenuScene {
    camera: Camera,
    background: Background,
    update_bg_on_enter: bool,
    scrolling_text_style: TextStyle,
    scrolling_text_offset: f32,
    character_data: CharacterData,
    navigation_menu: NavigationMenu,
    textbox: Textbox,
    event_receiver: flume::Receiver<Event>,
    event_sender: flume::Sender<Event>,
    quitting: bool,
    next_scene: NextScene,
}

impl MainMenuScene {
    pub fn new(game_io: &mut GameIO) -> MainMenuScene {
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;

        // layout
        let mut ui_animator = Animator::load_new(assets, ResourcePaths::MAIN_MENU_UI_ANIMATION);
        ui_animator.set_state("DEFAULT");
        let scrolling_text_point = ui_animator.point_or_zero("SCROLLING_TEXT");

        // scrolling text
        let mut scrolling_text_style = TextStyle::new(game_io, FontName::Context);
        scrolling_text_style.bounds.y = scrolling_text_point.y;

        // character data
        let character_data = CharacterData::load(game_io, &scrolling_text_style);

        // navigation
        let navigation_options = character_data.navigation_options();

        let mut textbox = Textbox::new_navigation(game_io);

        if !character_data.loaded {
            let globals = Globals::from_resources(game_io);
            let initial_setup_message = globals.translate("initial-setup-message");

            textbox.use_navigation_avatar(game_io);
            textbox.push_interface(TextboxMessage::new(initial_setup_message));
            textbox.open();
        }

        // events
        let (event_sender, event_receiver) = flume::unbounded();

        MainMenuScene {
            camera: Camera::new_ui(game_io),
            background: Background::new_blank(game_io),
            update_bg_on_enter: true,
            scrolling_text_style,
            scrolling_text_offset: 0.0,
            character_data,
            navigation_menu: NavigationMenu::new(game_io, navigation_options),
            textbox,
            event_sender,
            event_receiver,
            quitting: false,
            next_scene: NextScene::None,
        }
    }

    pub fn set_background(&mut self, background: Background) {
        self.background = background;
        self.update_bg_on_enter = false;
    }
}

impl Scene for MainMenuScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn enter(&mut self, game_io: &mut GameIO) {
        // reload character
        let previously_loaded = self.character_data.loaded;
        self.character_data = CharacterData::load(game_io, &self.scrolling_text_style);

        if self.character_data.loaded != previously_loaded {
            // restrict navigation depending on if the character is loaded
            self.navigation_menu =
                NavigationMenu::new(game_io, self.character_data.navigation_options());
        }

        // update character in the textbox
        self.textbox.use_navigation_avatar(game_io);

        // can't be on a server if the player is viewing the main menu
        let globals = Globals::from_resources_mut(game_io);
        globals.connected_to_server = false;

        // update the background if it's necessary
        if self.update_bg_on_enter {
            self.background = Background::new_main_menu(game_io);
        }

        // queue the background to update when we enter again
        self.update_bg_on_enter = true;
    }

    fn update(&mut self, game_io: &mut GameIO) {
        // handle events
        if let Ok(Event::Quit) = self.event_receiver.try_recv() {
            self.quitting = true;
        }

        if !self.textbox.is_open() && self.quitting {
            game_io.quit();
        }

        // music
        let globals = Globals::from_resources(game_io);

        if !game_io.is_in_transition() && !globals.audio.is_music_playing() {
            globals.audio.pick_music(&globals.music.main_menu, true);
        }

        // update background
        self.background.update();

        // update textbox
        if self.textbox.is_complete() {
            self.textbox.close();
        }

        self.textbox.update(game_io);

        // update scrolling text
        self.scrolling_text_offset -= 1.0;
        self.scrolling_text_offset %= self.character_data.scrolling_text_wrap;

        // handle input
        if self.textbox.is_open() && !self.navigation_menu.opening() {
            return;
        }

        self.next_scene = self.navigation_menu.update(game_io, |_, _| None);

        // handle more input if the navigation menu allows it
        if self.next_scene.is_some() || self.navigation_menu.opening() {
            return;
        }

        let input_util = InputUtil::new(game_io);

        if input_util.was_just_pressed(Input::Cancel) {
            let globals = Globals::from_resources(game_io);
            let event_sender = self.event_sender.clone();
            let message = globals.translate("navigation-quit-question");
            let interface = TextboxQuestion::new(game_io, message, move |yes| {
                if yes {
                    let _ = event_sender.send(Event::Quit);
                }
            });
            self.textbox.push_interface(interface);
            self.textbox.open();
        }
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        // draw background
        self.background.draw(game_io, render_pass);

        // draw ui
        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        // draw scrolling text
        self.scrolling_text_style.bounds.x = self.scrolling_text_offset;
        self.scrolling_text_style.draw(
            game_io,
            &mut sprite_queue,
            &self.character_data.scrolling_text,
        );

        // draw character
        sprite_queue.draw_sprite(&self.character_data.shadow_sprite);
        sprite_queue.draw_sprite(&self.character_data.sprite);

        // draw navigation
        self.navigation_menu.draw(game_io, &mut sprite_queue);

        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}
