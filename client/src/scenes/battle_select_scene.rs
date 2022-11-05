use crate::battle::BattleProps;
use crate::bindable::SpriteColorMode;
use crate::packages::*;
use crate::render::ui::{SceneTitle, Textbox, TextboxMessage};
use crate::render::*;
use crate::resources::*;
use crate::scenes::BattleScene;
use framework::prelude::*;

pub struct BattleSelectScene {
    camera: Camera,
    background: Background,
    selection: usize,
    package_ids: Vec<String>,
    preview_sprite: Sprite,
    textbox: Textbox,
    next_scene: NextScene<Globals>,
}

impl BattleSelectScene {
    pub fn new(game_io: &mut GameIO<Globals>) -> Box<Self> {
        let globals = game_io.globals();
        let assets = &globals.assets;

        let mut camera = Camera::new(game_io);
        camera.snap(RESOLUTION_F * 0.5);

        // background
        let background_animator = Animator::new();
        let background_sprite = assets.new_sprite(game_io, ResourcePaths::BATTLE_SELECT_BG);

        // preview
        let battle_manager = &globals.battle_packages;
        let package_ids: Vec<_> = battle_manager.local_packages().cloned().collect();

        let mut preview_sprite = assets.new_sprite(game_io, ResourcePaths::BLANK);
        preview_sprite.set_position(Vec2::new(12.0 + 44.0, 36.0 + 28.0));

        let mut scene = Box::new(Self {
            camera,
            background: Background::new(background_animator, background_sprite),
            selection: 0,
            package_ids,
            preview_sprite,
            textbox: Textbox::new_navigation(game_io)
                .with_accept_input(false)
                .begin_open(),
            next_scene: NextScene::None,
        });

        scene.update_preview(game_io);

        scene
    }
}

impl BattleSelectScene {
    fn update_preview(&mut self, game_io: &GameIO<Globals>) {
        if self.package_ids.is_empty() {
            return;
        }

        let package_id = &self.package_ids[self.selection];

        let globals = game_io.globals();
        let assets = &globals.assets;
        let battle_manager = &globals.battle_packages;

        let package = battle_manager
            .package(PackageNamespace::Local, package_id)
            .unwrap();

        let preview_texture = assets.texture(game_io, &package.preview_texture_path);
        self.preview_sprite.set_texture(preview_texture);

        let textbox_interface =
            TextboxMessage::new_auto(package.description.clone()).with_completable(false);

        self.textbox.advance_interface(game_io);
        self.textbox.push_interface(textbox_interface);
    }
}

impl Scene<Globals> for BattleSelectScene {
    fn next_scene(&mut self) -> &mut NextScene<Globals> {
        &mut self.next_scene
    }

    fn update(&mut self, game_io: &mut GameIO<Globals>) {
        self.camera.update(game_io);

        self.textbox.update(game_io);

        if game_io.is_in_transition() {
            return;
        }

        let input_util = InputUtil::new(game_io);
        let old_selection = self.selection;

        if input_util.was_just_pressed(Input::Left) {
            if self.selection == 0 {
                self.selection = self.package_ids.len().max(1) - 1;
            } else {
                self.selection -= 1;
            }
        }

        if input_util.was_just_pressed(Input::Right) {
            self.selection = (self.selection + 1) % self.package_ids.len();
        }

        if old_selection != self.selection {
            self.update_preview(game_io);

            let globals = game_io.globals();
            globals.audio.play_sound(&globals.cursor_move_sfx);
        }

        if input_util.was_just_pressed(Input::Confirm) && !self.package_ids.is_empty() {
            use crate::transitions::{ColorFadeTransition, DEFAULT_FADE_DURATION};

            let globals = game_io.globals();

            let package_id = &self.package_ids[self.selection];
            let battle_package = globals
                .battle_packages
                .package_or_fallback(PackageNamespace::Server, package_id);

            let props = BattleProps::new_with_defaults(game_io, battle_package);
            let scene = BattleScene::new(game_io, props);
            let transition = ColorFadeTransition::new(game_io, Color::WHITE, DEFAULT_FADE_DURATION);

            self.next_scene = NextScene::new_push(scene).with_transition(transition);
        }

        if input_util.was_just_pressed(Input::Cancel) {
            let globals = game_io.globals();
            globals.audio.play_sound(&globals.menu_close_sfx);

            use crate::transitions::{PushTransition, DEFAULT_PUSH_DURATION};

            let sampler = game_io.globals().default_sampler.clone();
            let transition =
                PushTransition::new(game_io, sampler, Direction::Left, DEFAULT_PUSH_DURATION);

            self.next_scene = NextScene::new_pop().with_transition(transition);
        }
    }

    fn draw(&mut self, game_io: &mut GameIO<Globals>, render_pass: &mut RenderPass) {
        // draw background
        self.background.draw(game_io, render_pass);

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        // draw title
        SceneTitle::new("BATTLE SELECT").draw(game_io, &mut sprite_queue);

        // draw preview sprite
        let preview_size = self.preview_sprite.size();
        self.preview_sprite.set_origin(preview_size * 0.5);
        sprite_queue.draw_sprite(&self.preview_sprite);

        // draw textbox
        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}
