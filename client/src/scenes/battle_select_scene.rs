use crate::battle::BattleProps;
use crate::bindable::SpriteColorMode;
use crate::packages::*;
use crate::render::ui::{SceneTitle, SubSceneFrame, Textbox, TextboxMessage};
use crate::render::*;
use crate::resources::*;
use crate::scenes::BattleScene;
use framework::prelude::*;

pub struct BattleSelectScene {
    camera: Camera,
    background: Background,
    frame: SubSceneFrame,
    selection: usize,
    package_ids: Vec<PackageId>,
    preview_sprite: Sprite,
    textbox: Textbox,
    next_scene: NextScene,
    title_text: String,
}

impl BattleSelectScene {
    pub fn new(game_io: &mut GameIO) -> Box<Self> {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let mut camera = Camera::new(game_io);
        camera.snap(RESOLUTION_F * 0.5);

        // preview
        let mut preview_sprite = assets.new_sprite(game_io, ResourcePaths::BLANK);
        preview_sprite.set_position(Vec2::new(12.0 + 44.0, 36.0 + 28.0));

        // package list
        let encounter_manager = &globals.encounter_packages;
        let mut package_ids: Vec<_> = encounter_manager
            .package_ids(PackageNamespace::Local)
            .cloned()
            .collect();

        package_ids.sort();

        let mut scene = Box::new(Self {
            camera,
            background: Background::new_sub_scene(game_io),
            frame: SubSceneFrame::new(game_io).with_top_bar(true),
            selection: 0,
            package_ids,
            preview_sprite,
            textbox: Textbox::new_navigation(game_io)
                .with_accept_input(false)
                .begin_open(),
            next_scene: NextScene::None,
            title_text: String::from("BATTLE SELECT: "),
        });

        scene.update_preview(game_io);

        scene
    }
}

impl BattleSelectScene {
    fn update_preview(&mut self, game_io: &GameIO) {
        if self.package_ids.is_empty() {
            return;
        }

        let package_id = &self.package_ids[self.selection];

        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;
        let encounter_manager = &globals.encounter_packages;

        let package = encounter_manager
            .package(PackageNamespace::Local, package_id)
            .unwrap();

        let package_name = package.name.to_uppercase();
        if !package.name.is_empty() {
            self.title_text = format!("BATTLE SELECT: {package_name}");
        } else {
            self.title_text = String::from("BATTLE SELECT: ???");
        }

        let preview_texture = assets.texture(game_io, &package.preview_texture_path);
        self.preview_sprite.set_texture(preview_texture);

        let textbox_interface =
            TextboxMessage::new_auto(package.description.clone()).with_completable(false);

        self.textbox.advance_interface(game_io);
        self.textbox.push_interface(textbox_interface);
    }

    fn handle_music(&self, game_io: &GameIO) {
        let globals = game_io.resource::<Globals>().unwrap();

        if !globals.audio.is_music_playing() {
            globals.audio.restart_music();
        }
    }
}

impl Scene for BattleSelectScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn update(&mut self, game_io: &mut GameIO) {
        self.background.update();
        self.camera.update(game_io);

        self.textbox.update(game_io);

        if game_io.is_in_transition() {
            return;
        }

        self.handle_music(game_io);

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
            self.selection += 1;
            self.selection = (self.selection)
                .checked_rem(self.package_ids.len())
                .unwrap_or_default();
        }

        if old_selection != self.selection {
            self.update_preview(game_io);

            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_move);
        }

        if input_util.was_just_pressed(Input::Confirm) && !self.package_ids.is_empty() {
            let globals = game_io.resource::<Globals>().unwrap();

            // play sfx
            globals.audio.stop_music();
            globals.audio.play_sound(&globals.sfx.battle_transition);

            // get the battle
            let package_id = &self.package_ids[self.selection];
            let encounter_package = globals
                .encounter_packages
                .package_or_fallback(PackageNamespace::Local, package_id);

            let props = BattleProps::new_with_defaults(game_io, encounter_package);

            // set the next scene
            let scene = BattleScene::new(game_io, props);
            let transition = crate::transitions::new_battle(game_io);
            self.next_scene = NextScene::new_push(scene).with_transition(transition);
        }

        if input_util.was_just_pressed(Input::Cancel) {
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_cancel);

            let transition = crate::transitions::new_scene_pop(game_io);
            self.next_scene = NextScene::new_pop().with_transition(transition);
        }
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        // draw background
        self.background.draw(game_io, render_pass);

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        // draw title
        self.frame.draw(&mut sprite_queue);
        SceneTitle::new(self.title_text.as_str()).draw(game_io, &mut sprite_queue);

        // draw preview sprite
        let preview_size = self.preview_sprite.size();
        self.preview_sprite.set_origin(preview_size * 0.5);
        sprite_queue.draw_sprite(&self.preview_sprite);

        // draw textbox
        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}
