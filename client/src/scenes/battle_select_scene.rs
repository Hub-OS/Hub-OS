use crate::battle::BattleProps;
use crate::bindable::SpriteColorMode;
use crate::packages::*;
use crate::render::ui::{
    GridScrollTracker, SceneTitle, SubSceneFrame, Textbox, TextboxMessage, UiInputTracker,
};
use crate::render::*;
use crate::resources::*;
use crate::scenes::BattleInitScene;
use framework::prelude::*;

pub struct BattleSelectScene {
    camera: Camera,
    background: Background,
    frame: SubSceneFrame,
    ui_input_tracker: UiInputTracker,
    preview_frame_sprite: Sprite,
    recording_frame_sprite: Sprite,
    cursor_sprite: Sprite,
    cursor_animator: Animator,
    scroll_tracker: GridScrollTracker,
    package_ids: Vec<PackageId>,
    textbox: Textbox,
    next_scene: NextScene,
    title_text: String,
}

impl BattleSelectScene {
    pub fn new(game_io: &GameIO) -> Box<Self> {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        // layout
        let mut ui_animator = Animator::load_new(assets, ResourcePaths::BATTLE_SELECT_UI_ANIMATION);
        ui_animator.set_state("DEFAULT");
        let grid_start = ui_animator.point_or_zero("GRID_START");
        let grid_step = ui_animator.point_or_zero("GRID_STEP");

        // preview frame
        let mut preview_frame_sprite = assets.new_sprite(game_io, ResourcePaths::BATTLE_SELECT_UI);
        ui_animator.set_state("FRAME");
        ui_animator.apply(&mut preview_frame_sprite);

        // recorded frame
        let mut recording_frame_sprite = preview_frame_sprite.clone();
        ui_animator.set_state("RECORDING_FRAME");
        ui_animator.apply(&mut recording_frame_sprite);

        // cursor sprite
        let mut cursor_sprite = preview_frame_sprite.clone();
        ui_animator.set_state("CURSOR");
        ui_animator.set_loop_mode(AnimatorLoopMode::Loop);
        ui_animator.apply(&mut cursor_sprite);

        let mut scene = Box::new(Self {
            camera: Camera::new_ui(game_io),
            background: Background::new_sub_scene(game_io),
            frame: SubSceneFrame::new(game_io).with_top_bar(true),
            preview_frame_sprite,
            recording_frame_sprite,
            cursor_sprite,
            cursor_animator: ui_animator,
            ui_input_tracker: UiInputTracker::new(),
            scroll_tracker: GridScrollTracker::new(game_io, 3, 2)
                .with_position(grid_start)
                .with_step(grid_step)
                .with_view_margin(1),
            package_ids: Vec::new(),
            textbox: Textbox::new_navigation(game_io),
            next_scene: NextScene::None,
            title_text: String::from("BATTLE SELECT: "),
        });

        scene.update_title(game_io);

        scene
    }
}

impl BattleSelectScene {
    fn update_title(&mut self, game_io: &GameIO) {
        if self.package_ids.is_empty() {
            return;
        }

        let selected_index = self.scroll_tracker.selected_index();
        let package_id = &self.package_ids[selected_index];

        let globals = game_io.resource::<Globals>().unwrap();
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

    fn enter(&mut self, game_io: &mut GameIO) {
        // reload package list
        let globals = game_io.resource::<Globals>().unwrap();
        let encounter_manager = &globals.encounter_packages;
        let mut package_ids: Vec<_> = encounter_manager
            .package_ids(PackageNamespace::Local)
            .cloned()
            .collect();

        package_ids.sort();

        self.scroll_tracker.set_total_items(package_ids.len());
        self.package_ids = package_ids;

        // update title to match the possibly changed selection
        self.update_title(game_io);
    }

    fn update(&mut self, game_io: &mut GameIO) {
        self.background.update();
        self.camera.update(game_io);
        self.ui_input_tracker.update(game_io);
        self.cursor_animator.update();
        self.cursor_animator.apply(&mut self.cursor_sprite);

        self.textbox.update(game_io);

        if self.textbox.is_complete() {
            self.textbox.close()
        }

        if game_io.is_in_transition() {
            return;
        }

        self.handle_music(game_io);

        if self.textbox.is_open() {
            return;
        }

        let input_tracker = &self.ui_input_tracker;
        let prev_index = self.scroll_tracker.selected_index();

        self.scroll_tracker.handle_input(input_tracker);

        let index = self.scroll_tracker.selected_index();

        if prev_index != index {
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_move);

            self.update_title(game_io);
        }

        let input_tracker = &self.ui_input_tracker;

        if input_tracker.pulsed(Input::Confirm) && !self.package_ids.is_empty() {
            let package_id = self.package_ids[index].clone();
            let encounter_package = Some((PackageNamespace::Local, package_id));

            // set the next scene
            let props = BattleProps::new_with_defaults(game_io, encounter_package);
            let scene = BattleInitScene::new(game_io, props);

            let transition = crate::transitions::new_battle(game_io);
            self.next_scene = NextScene::new_push(scene).with_transition(transition);
            return;
        }

        if input_tracker.pulsed(Input::Option2) && !self.package_ids.is_empty() {
            // description
            let globals = game_io.resource::<Globals>().unwrap();

            let i = self.scroll_tracker.selected_index();
            let package_id = &self.package_ids[i];
            let package = globals
                .encounter_packages
                .package(PackageNamespace::Local, package_id)
                .unwrap();

            let interface = TextboxMessage::new(package.description.clone());

            self.textbox.push_interface(interface);
            self.textbox.open();
            return;
        }

        if self.ui_input_tracker.pulsed(Input::Cancel) {
            // leave
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

        // draw previews
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        for (i, position) in self.scroll_tracker.iter_visible() {
            // get package
            let package_id = &self.package_ids[i];
            let package = globals
                .encounter_packages
                .package(PackageNamespace::Local, package_id)
                .unwrap();

            // draw frame
            if package.recording_path.is_some() {
                //  recording frame
                self.recording_frame_sprite.set_position(position);
                sprite_queue.draw_sprite(&self.recording_frame_sprite);
            } else {
                // normal frame
                self.preview_frame_sprite.set_position(position);
                sprite_queue.draw_sprite(&self.preview_frame_sprite);
            }

            // draw package icon
            let mut preview_sprite = assets.new_sprite(game_io, &package.preview_texture_path);
            preview_sprite.set_origin(preview_sprite.size() * 0.5);
            preview_sprite.set_position(position);
            sprite_queue.draw_sprite(&preview_sprite);
        }

        // draw cursor
        if !self.package_ids.is_empty() {
            let selected_index = self.scroll_tracker.selected_index();
            let cursor_position = self.scroll_tracker.index_position(selected_index);
            self.cursor_sprite.set_position(cursor_position);
            sprite_queue.draw_sprite(&self.cursor_sprite);
        }

        // draw frame and title
        self.frame.draw(&mut sprite_queue);
        SceneTitle::new(self.title_text.as_str()).draw(game_io, &mut sprite_queue);

        // draw textbox
        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}
