use super::FolderEditScene;
use crate::bindable::SpriteColorMode;
use crate::render::ui::*;
use crate::render::*;
use crate::resources::*;
use crate::saves::Folder;
use framework::prelude::*;

enum Event {
    Rename(String),
    Delete,
    CloseTextbox,
}

#[derive(Clone, Copy)]
enum FolderOption {
    Edit,
    Equip,
    ChangeName,
    New,
    Delete,
}

pub struct FolderListScene {
    camera: Camera,
    background: Background,
    ui_input_tracker: UiInputTracker,
    scene_time: FrameTime,
    equipped_sprite: Sprite,
    equipped_animator: Animator,
    folder_scroll_offset: f32,
    folder_sprite: Sprite,
    folder_start_position: Vec2,
    folder_scroll_tracker: ScrollTracker,
    card_scroll_tracker: ScrollTracker,
    card_list_position: Vec2,
    context_menu: ContextMenu<FolderOption>,
    event_sender: flume::Sender<Event>,
    event_receiver: flume::Receiver<Event>,
    textbox: Textbox,
    next_scene: NextScene,
}

impl FolderListScene {
    pub fn new(game_io: &mut GameIO) -> Box<Self> {
        move_selected_folder(game_io);

        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let mut camera = Camera::new(game_io);
        camera.snap(RESOLUTION_F * 0.5);

        // background
        let mut layout_animator =
            Animator::load_new(assets, ResourcePaths::FOLDERS_LAYOUT_ANIMATION);
        layout_animator.set_state("DEFAULT");

        // card scroll tracker
        let mut card_scroll_tracker = ScrollTracker::new(game_io, 5);

        let card_list_position = layout_animator.point("LIST").unwrap_or_default();

        let scroll_start = layout_animator.point("SCROLL_START").unwrap_or_default();
        let scroll_end = layout_animator.point("SCROLL_END").unwrap_or_default();

        card_scroll_tracker.define_scrollbar(scroll_start, scroll_end);

        // folder sprites
        let folder_sprite = assets.new_sprite(game_io, ResourcePaths::FOLDERS_ENABLED);

        let folder_start_position = layout_animator.point("FOLDER_START").unwrap_or_default();

        // folder cursor sprite
        let mut folder_scroll_tracker = ScrollTracker::new(game_io, 3);
        folder_scroll_tracker.use_custom_cursor(
            game_io,
            ResourcePaths::FOLDERS_CURSOR_ANIMATION,
            ResourcePaths::FOLDERS_CURSOR,
        );
        folder_scroll_tracker.set_vertical(false);

        let cursor_start = folder_start_position + Vec2::new(3.0, 7.0);
        folder_scroll_tracker.define_cursor(cursor_start, folder_sprite.size().x + 1.0);

        // equipped sprite
        let mut equipped_animator =
            Animator::load_new(assets, ResourcePaths::FOLDERS_EQUIPPED_ANIMATION);
        equipped_animator.set_state("BLINK");
        equipped_animator.set_loop_mode(AnimatorLoopMode::Loop);

        let equipped_sprite = assets.new_sprite(game_io, ResourcePaths::FOLDERS_EQUIPPED);

        let (event_sender, event_receiver) = flume::unbounded();

        Box::new(Self {
            camera,
            background: Background::load_static(game_io, ResourcePaths::FOLDERS_BG),
            ui_input_tracker: UiInputTracker::new(),
            scene_time: 0,
            equipped_sprite,
            equipped_animator,
            folder_scroll_offset: 0.0,
            folder_sprite,
            folder_start_position,
            folder_scroll_tracker,
            card_scroll_tracker,
            card_list_position,
            context_menu: ContextMenu::new(game_io, "SELECT", Vec2::new(3.0, 50.0))
                .with_arrow(true),
            event_sender,
            event_receiver,
            textbox: Textbox::new_navigation(game_io),
            next_scene: NextScene::None,
        })
    }
}

fn move_selected_folder(game_io: &mut GameIO) {
    let globals = game_io.resource_mut::<Globals>().unwrap();
    let save = &mut globals.global_save;

    if save.selected_folder == 0 {
        return;
    }

    let folder = save.folders.remove(save.selected_folder);
    save.folders.insert(0, folder);
    save.selected_folder = 0;
}

impl Scene for FolderListScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn enter(&mut self, game_io: &mut GameIO) {
        let folders = &game_io.resource::<Globals>().unwrap().global_save.folders;

        self.folder_scroll_tracker.set_total_items(folders.len());

        let selected_index = self.folder_scroll_tracker.selected_index();

        if let Some(folder) = folders.get(selected_index) {
            let count = folder.cards.len();
            self.card_scroll_tracker.set_total_items(count);
        }
    }

    fn update(&mut self, game_io: &mut GameIO) {
        self.scene_time += 1;
        self.camera.update(game_io);

        if game_io.is_in_transition() {
            return;
        }

        self.textbox.update(game_io);

        handle_events(self, game_io);

        if self.textbox.is_open() {
            return;
        }

        handle_input(self, game_io);
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        let globals = game_io.resource::<Globals>().unwrap();
        let global_save = &globals.global_save;

        // draw background
        self.background.draw(game_io, render_pass);

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        // draw title
        SceneTitle::new("FOLDERS").draw(game_io, &mut sprite_queue);

        // draw context menu
        self.context_menu.draw(game_io, &mut sprite_queue);

        // folder offset calculations
        let folder_top_index = self.folder_scroll_tracker.top_index();
        self.folder_scroll_offset += (folder_top_index as f32 - self.folder_scroll_offset) * 0.2;

        // draw folders
        const LABEL_OFFSET: Vec2 = Vec2::new(4.0, 11.0);

        let mut label =
            TextStyle::new(game_io, FontStyle::Thick).with_shadow_color(TEXT_DARK_SHADOW_COLOR);

        let selection_multiplier = self.folder_sprite.size().x + 1.0;

        let folder_offset = -self.folder_scroll_offset * selection_multiplier;

        for (i, folder) in global_save.folders.iter().enumerate() {
            let mut position = self.folder_start_position;
            position.x += i as f32 * selection_multiplier + folder_offset;

            // draw folder first
            self.folder_sprite.set_position(position);
            sprite_queue.draw_sprite(&self.folder_sprite);

            if i == global_save.selected_folder {
                self.equipped_animator.update();
                self.equipped_animator.apply(&mut self.equipped_sprite);

                self.equipped_sprite.set_position(position);
                sprite_queue.draw_sprite(&self.equipped_sprite);
            }

            // folder label
            label.bounds.set_position(position + LABEL_OFFSET);
            label.draw(game_io, &mut sprite_queue, &folder.name);
        }

        if !global_save.folders.is_empty() {
            // draw folder cursor
            self.folder_scroll_tracker.draw_cursor(&mut sprite_queue);

            // draw cards
            let folder_index = self.folder_scroll_tracker.selected_index();
            let cards = &global_save.folders[folder_index].cards;
            let range = self.card_scroll_tracker.view_range();
            let mut card_position = self.card_list_position;

            for i in range {
                cards[i].draw_list_item(game_io, &mut sprite_queue, card_position);
                card_position.y += 16.0;
            }

            // draw card cursor
            self.card_scroll_tracker.draw_scrollbar(&mut sprite_queue);
        }

        // draw textbox
        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}

fn handle_events(scene: &mut FolderListScene, game_io: &mut GameIO) {
    let global_save = &mut game_io.resource_mut::<Globals>().unwrap().global_save;

    match scene.event_receiver.try_recv() {
        Ok(Event::Rename(name)) => {
            let folder_index = scene.folder_scroll_tracker.selected_index();
            global_save.folders[folder_index].name = name;
            global_save.save();

            scene.textbox.close();
        }
        Ok(Event::Delete) => {
            let folder_index = scene.folder_scroll_tracker.selected_index();
            global_save.folders.remove(folder_index);

            let new_total = global_save.folders.len();
            scene.folder_scroll_tracker.set_total_items(new_total);

            if global_save.selected_folder <= new_total {
                global_save.selected_folder = 0;
            }

            global_save.save();

            // update card list
            let folder_index = scene.folder_scroll_tracker.selected_index();
            let card_count = match global_save.folders.get(folder_index) {
                Some(folder) => folder.cards.len(),
                None => 0,
            };

            scene.card_scroll_tracker.set_total_items(card_count);
            scene.textbox.close();
        }
        Ok(Event::CloseTextbox) => {
            scene.textbox.close();
        }
        Err(_) => {}
    }
}

fn handle_input(scene: &mut FolderListScene, game_io: &mut GameIO) {
    scene.ui_input_tracker.update(game_io);

    if scene.context_menu.is_open() {
        handle_context_menu_input(scene, game_io);
        return;
    }

    let globals = game_io.resource::<Globals>().unwrap();

    // folder scroll
    let folders = &globals.global_save.folders;
    let total_folders = folders.len();

    if total_folders > 0 {
        let previous_folder_index = scene.folder_scroll_tracker.selected_index();

        if scene.ui_input_tracker.is_active(Input::Left) {
            scene.folder_scroll_tracker.move_up();
        }

        if scene.ui_input_tracker.is_active(Input::Right) {
            scene.folder_scroll_tracker.move_down();
        }

        let folder_index = scene.folder_scroll_tracker.selected_index();

        if previous_folder_index != folder_index {
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.cursor_move_sfx);

            let count = folders[folder_index].cards.len();
            scene.card_scroll_tracker.set_total_items(count);
        }
    }

    // card scroll
    let previous_card_index = scene.card_scroll_tracker.selected_index();

    if scene.ui_input_tracker.is_active(Input::Up) {
        scene.card_scroll_tracker.move_view_up();
    }

    if scene.ui_input_tracker.is_active(Input::Down) {
        scene.card_scroll_tracker.move_view_down();
    }

    if scene.ui_input_tracker.is_active(Input::ShoulderL) {
        scene.card_scroll_tracker.page_up();
    }

    if scene.ui_input_tracker.is_active(Input::ShoulderR) {
        scene.card_scroll_tracker.page_down();
    }

    if previous_card_index != scene.card_scroll_tracker.selected_index() {
        let globals = game_io.resource::<Globals>().unwrap();
        globals.audio.play_sound(&globals.cursor_move_sfx);
    }

    // confirm + cancel
    let input_util = InputUtil::new(game_io);

    if input_util.was_just_pressed(Input::Cancel) {
        let globals = game_io.resource::<Globals>().unwrap();
        globals.audio.play_sound(&globals.cursor_cancel_sfx);

        let transition = crate::transitions::new_scene_pop(game_io);
        scene.next_scene = NextScene::new_pop().with_transition(transition);
        return;
    }

    if input_util.was_just_pressed(Input::Confirm) {
        let globals = game_io.resource::<Globals>().unwrap();
        globals.audio.play_sound(&globals.cursor_select_sfx);

        let options: &[(&str, FolderOption)] = if total_folders == 0 {
            &[("NEW", FolderOption::New)]
        } else {
            &[
                ("EDIT", FolderOption::Edit),
                ("EQUIP", FolderOption::Equip),
                ("CHG NAME", FolderOption::ChangeName),
                ("NEW", FolderOption::New),
                ("DELETE", FolderOption::Delete),
            ]
        };

        scene.context_menu.set_options(game_io, options);

        scene.context_menu.open();
    }
}

fn handle_context_menu_input(scene: &mut FolderListScene, game_io: &mut GameIO) {
    let selection = match scene.context_menu.update(game_io, &scene.ui_input_tracker) {
        Some(selection) => selection,
        None => return,
    };

    let globals = game_io.resource_mut::<Globals>().unwrap();
    let global_save = &mut globals.global_save;

    match selection {
        FolderOption::Edit => {
            let folder_index = scene.folder_scroll_tracker.selected_index();

            scene.next_scene = NextScene::new_push(FolderEditScene::new(game_io, folder_index))
                .with_transition(crate::transitions::new_sub_scene(game_io));
        }
        FolderOption::Equip => {
            global_save.selected_folder = scene.folder_scroll_tracker.selected_index();
            global_save.save();
        }
        FolderOption::ChangeName => {
            let event_sender = scene.event_sender.clone();
            let callback = move |name: String| {
                let event = if !name.is_empty() {
                    Event::Rename(name)
                } else {
                    Event::CloseTextbox
                };

                event_sender.send(event).unwrap();
            };

            let folder_name = &global_save.folders[global_save.selected_folder].name;
            let textbox_interface = TextboxPrompt::new(callback)
                .with_str(folder_name)
                .with_character_limit(9);

            scene.textbox.push_interface(textbox_interface);
            scene.textbox.open();
        }
        FolderOption::New => {
            let name = String::from("NewFldr");
            global_save.folders.push(Folder::new(name));

            let total_folders = global_save.folders.len();

            scene.folder_scroll_tracker.set_total_items(total_folders);

            scene
                .folder_scroll_tracker
                .set_selected_index(total_folders - 1);
            scene.card_scroll_tracker.set_total_items(0);
        }
        FolderOption::Delete => {
            let event_sender = scene.event_sender.clone();
            let folder_name = &global_save.folders[global_save.selected_folder].name;
            let callback = move |response| {
                let event = if response {
                    Event::Delete
                } else {
                    Event::CloseTextbox
                };

                event_sender.send(event).unwrap();
            };

            let textbox_interface =
                TextboxQuestion::new(format!("Delete {folder_name}?"), callback);

            scene.textbox.push_interface(textbox_interface);
            scene.textbox.open();
        }
    }

    scene.context_menu.close();
}
