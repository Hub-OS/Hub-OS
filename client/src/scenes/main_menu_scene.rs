use crate::bindable::SpriteColorMode;
use crate::packages::PackageNamespace;
use crate::render::ui::{FontName, NavigationMenu, SceneOption, TextStyle};
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;
use ui::{Textbox, TextboxMessage};

struct CharacterData {
    loaded: bool,
    scrolling_text: String,
    scrolling_text_wrap: f32,
    sprite: Sprite,
    shadow_sprite: Sprite,
}

impl CharacterData {
    fn init_selected_player(game_io: &mut GameIO) {
        let globals = game_io.resource_mut::<Globals>().unwrap();

        let player_packages = &globals.player_packages;
        let has_selected_character = player_packages
            .package_ids(PackageNamespace::Local)
            .any(|player_id| *player_id == globals.global_save.selected_character);

        if has_selected_character {
            return;
        }

        let Some(package_id) = player_packages.package_ids(PackageNamespace::Local).next() else {
            return;
        };

        let global_save = &mut globals.global_save;
        global_save.selected_character = package_id.clone();
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

        let globals = game_io.resource::<Globals>().unwrap();
        let player_id = &globals.global_save.selected_character;
        let assets = &globals.assets;

        let player_package = globals
            .player_packages
            .package(PackageNamespace::Local, player_id);

        // sprite
        let mut sprite = if let Some(package) = player_package {
            assets.new_sprite(game_io, &package.preview_texture_path)
        } else {
            assets.new_sprite(game_io, ResourcePaths::WHITE_PIXEL)
        };

        sprite.set_position(Vec2::new(0.0, RESOLUTION_F.y - sprite.size().y));

        let mut shadow_sprite = sprite.clone();

        shadow_sprite.set_origin(Vec2::new(20.0, 0.0));
        shadow_sprite.set_color(Color::new(0.0, 0.0, 0.0, 0.3));

        // text
        let part_text = player_package
            .map(|package| package.name.as_str())
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
    next_scene: NextScene,
}

impl MainMenuScene {
    pub fn new(game_io: &mut GameIO) -> MainMenuScene {
        let globals = game_io.resource::<Globals>().unwrap();
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
            const MESSAGE: &str = "Missing player mod.\n\n\nInstall from Manage Mods in Config.";

            textbox.use_navigation_avatar(game_io);
            textbox.push_interface(TextboxMessage::new(MESSAGE.to_string()));
            textbox.open();
        }

        MainMenuScene {
            camera: Camera::new_ui(game_io),
            background: Background::new_blank(game_io),
            update_bg_on_enter: true,
            scrolling_text_style,
            scrolling_text_offset: 0.0,
            character_data,
            navigation_menu: NavigationMenu::new(game_io, navigation_options),
            textbox,
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

        // can't be on a server if the player is viewing the main menu
        let globals = game_io.resource_mut::<Globals>().unwrap();
        globals.connected_to_server = false;

        // update the background if it's necessary
        if self.update_bg_on_enter {
            self.background = Background::new_main_menu(game_io);
        }

        // queue the background to update when we enter again
        self.update_bg_on_enter = true;
    }

    fn update(&mut self, game_io: &mut GameIO) {
        // music
        let globals = game_io.resource::<Globals>().unwrap();

        if !game_io.is_in_transition() && !globals.audio.is_music_playing() {
            globals.audio.play_music(&globals.music.main_menu, true);
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

        // update navigation menu
        if !self.textbox.is_open() || self.navigation_menu.opening() {
            self.next_scene = self.navigation_menu.update(game_io, |_, _| None);
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
