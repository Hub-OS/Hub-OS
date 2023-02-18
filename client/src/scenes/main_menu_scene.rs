use crate::bindable::SpriteColorMode;
use crate::packages::PackageNamespace;
use crate::render::ui::{FontStyle, NavigationMenu, SceneOption, TextStyle};
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;

struct CharacterData {
    scrolling_text: String,
    scrolling_text_wrap: f32,
    sprite: Sprite,
    shadow_sprite: Sprite,
}

impl CharacterData {
    fn load(game_io: &GameIO, text_style: &TextStyle) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let player_id = &globals.global_save.selected_character;
        let player_package = globals
            .player_packages
            .package(PackageNamespace::Local, player_id)
            .unwrap();

        // sprite
        let mut sprite = assets.new_sprite(game_io, &player_package.preview_texture_path);
        sprite.set_position(Vec2::new(0.0, RESOLUTION_F.y - sprite.size().y));

        let mut shadow_sprite = sprite.clone();

        shadow_sprite.set_origin(Vec2::new(20.0, 0.0));
        shadow_sprite.set_color(Color::new(0.0, 0.0, 0.0, 0.3));

        // text
        let part_text = player_package.name.to_uppercase() + "   ";
        let part_width = text_style.measure(&part_text).size.x + text_style.letter_spacing;
        let part_repeats = (RESOLUTION_F.x / part_width).ceil() as usize + 1;

        let scrolling_text = part_text.repeat(part_repeats);

        Self {
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
    scrolling_text_style: TextStyle,
    scrolling_text_offset: f32,
    character_data: CharacterData,
    navigation_menu: NavigationMenu,
    next_scene: NextScene,
}

impl MainMenuScene {
    pub fn new(game_io: &mut GameIO) -> MainMenuScene {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;
        let background_sampler = &globals.background_sampler;

        // setup camera
        let mut camera = Camera::new(game_io);
        camera.snap(RESOLUTION_F * 0.5);

        // background
        let background_texture = assets.texture(game_io, ResourcePaths::MAIN_MENU_BG);
        let background_sprite =
            Sprite::new_with_sampler(background_texture, background_sampler.clone());
        let background = Background::new(Animator::new(), background_sprite);

        // scrolling text
        let mut scrolling_text_style = TextStyle::new(game_io, FontStyle::Context);
        scrolling_text_style.bounds.y = RESOLUTION_F.y * 0.5;

        // character data
        let character_data = CharacterData::load(game_io, &scrolling_text_style);

        MainMenuScene {
            camera,
            background,
            scrolling_text_style,
            scrolling_text_offset: 0.0,
            character_data,
            navigation_menu: NavigationMenu::new(
                game_io,
                vec![
                    SceneOption::Servers,
                    SceneOption::Folders,
                    SceneOption::Library,
                    SceneOption::Character,
                    SceneOption::BattleSelect,
                    SceneOption::Config,
                ],
            ),
            next_scene: NextScene::None,
        }
    }
}

impl Scene for MainMenuScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn enter(&mut self, game_io: &mut GameIO) {
        // reload character
        self.character_data = CharacterData::load(game_io, &self.scrolling_text_style);

        // can't be on a server if the player is viewing the main menu
        let globals = game_io.resource_mut::<Globals>().unwrap();
        globals.connected_to_server = false;
    }

    fn update(&mut self, game_io: &mut GameIO) {
        // music
        let globals = game_io.resource::<Globals>().unwrap();

        if !game_io.is_in_transition() && !globals.audio.is_music_playing() {
            globals.audio.play_music(&globals.main_menu_music, true);
        }

        // ui
        self.background.update();

        self.scrolling_text_offset -= 1.0;
        self.scrolling_text_offset %= self.character_data.scrolling_text_wrap;

        self.next_scene = self.navigation_menu.update(game_io);
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        // draw background
        self.background.draw(game_io, render_pass);

        // draw ui
        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        // draw scrolling text

        self.scrolling_text_style.bounds.x = self.scrolling_text_offset;
        self.scrolling_text_style.bounds.y = RESOLUTION_F.y * 0.5;
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

        render_pass.consume_queue(sprite_queue);
    }
}
