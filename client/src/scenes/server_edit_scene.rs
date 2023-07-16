use crate::bindable::GenerationalIndex;
use crate::bindable::SpriteColorMode;
use crate::render::ui::*;
use crate::render::*;
use crate::resources::*;
use crate::saves::ServerInfo;
use framework::prelude::*;

pub enum ServerEditProp {
    Edit(usize),
    InsertAfter(usize),
}

enum UiMessage {
    NameUpdated(String),
    AddressUpdated(String),
    Cancel,
    Save,
}

pub struct ServerEditScene {
    camera: Camera,
    background: Background,
    frame: SubSceneFrame,
    edit_prop: ServerEditProp,
    server_info: ServerInfo,
    button_9patch: NinePatch,
    input_9patch: NinePatch,
    cursor_sprite: Sprite,
    cursor_animator: Animator,
    ui_input_tracker: UiInputTracker,
    ui_layout: UiLayout,
    ui_receiver: flume::Receiver<UiMessage>,
    save_handle: GenerationalIndex,
    next_scene: NextScene,
}

impl ServerEditScene {
    pub fn new(game_io: &GameIO, edit_prop: ServerEditProp) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();

        let server_info = match edit_prop {
            ServerEditProp::Edit(index) => globals.global_save.server_list[index].clone(),
            _ => ServerInfo {
                name: String::from("New Server"),
                address: String::from("localhost:8765"),
            },
        };

        // ui
        let assets = &globals.assets;

        // define styles
        let ui_texture = assets.texture(game_io, ResourcePaths::UI_NINE_PATCHES);
        let ui_animator = Animator::load_new(assets, ResourcePaths::UI_NINE_PATCHES_ANIMATION);
        let button_9patch = build_9patch!(game_io, ui_texture.clone(), &ui_animator, "BUTTON");
        let input_9patch = build_9patch!(game_io, ui_texture, &ui_animator, "TEXT_INPUT");

        let label_style = UiStyle {
            align_self: Some(AlignSelf::FlexStart),
            margin_left: LengthPercentageAuto::Points(3.0),
            margin_bottom: LengthPercentageAuto::Points(2.0),
            ..Default::default()
        };

        let button_style = UiStyle {
            margin_top: LengthPercentageAuto::Auto,
            margin_right: LengthPercentageAuto::Points(2.0),
            nine_patch: Some(button_9patch.clone()),
            ..Default::default()
        };

        let input_style = UiStyle {
            margin_bottom: LengthPercentageAuto::Points(2.0),
            padding_top: 1.0,
            padding_left: 3.0,
            padding_right: 3.0,
            min_height: Dimension::Points(20.0),
            max_height: Dimension::Points(20.0),
            max_width: Dimension::Percent(1.0),
            nine_patch: Some(input_9patch.clone()),
            ..Default::default()
        };

        // define ui with callbacks
        let (ui_sender, ui_receiver) = flume::unbounded();

        let mut save_handle = None;

        let ui_layout = UiLayout::new_vertical(
            Rect::new(8.0, 20.0, RESOLUTION_F.x - 16.0, RESOLUTION_F.y - 28.0),
            vec![
                // name input
                UiLayoutNode::new(
                    Text::new(game_io, FontStyle::Thick)
                        .with_str("Name")
                        .with_shadow_color(TEXT_DARK_SHADOW_COLOR),
                )
                .with_style(label_style.clone()),
                UiLayoutNode::new(
                    TextInput::new(game_io, FontStyle::Thin)
                        .with_str(&server_info.name)
                        .with_character_limit(20)
                        .on_change({
                            let sender = ui_sender.clone();

                            move |text| {
                                sender
                                    .send(UiMessage::NameUpdated(text.to_string()))
                                    .unwrap();
                            }
                        }),
                )
                .with_style(input_style.clone()),
                // address input
                UiLayoutNode::new(
                    Text::new(game_io, FontStyle::Thick)
                        .with_str("Address")
                        .with_shadow_color(TEXT_DARK_SHADOW_COLOR),
                )
                .with_style(label_style),
                UiLayoutNode::new(
                    TextInput::new(game_io, FontStyle::Thin)
                        .with_str(&server_info.address)
                        .with_shadow_color(TEXT_DARK_SHADOW_COLOR)
                        .on_change({
                            let sender = ui_sender.clone();

                            move |text| {
                                sender
                                    .send(UiMessage::AddressUpdated(text.to_string()))
                                    .unwrap();
                            }
                        }),
                )
                .with_style(input_style),
                // buttons
                UiLayoutNode::new_container()
                    .with_style(UiStyle {
                        flex_direction: FlexDirection::RowReverse,
                        flex_grow: 1.0,
                        align_items: Some(AlignItems::FlexEnd),
                        ..Default::default()
                    })
                    .with_children(vec![
                        UiLayoutNode::new(
                            UiButton::new_text(game_io, FontStyle::Thick, "Save").on_activate({
                                let sender = ui_sender.clone();

                                move || sender.send(UiMessage::Save).unwrap()
                            }),
                        )
                        .with_style(button_style.clone())
                        .with_handle(&mut save_handle),
                        UiLayoutNode::new(
                            UiButton::new_text(game_io, FontStyle::Thick, "Cancel").on_activate({
                                let sender = ui_sender;

                                move || sender.send(UiMessage::Cancel).unwrap()
                            }),
                        )
                        .with_style(button_style),
                    ]),
            ],
        );

        // cursor
        let cursor_sprite = assets.new_sprite(game_io, ResourcePaths::SELECT_CURSOR);

        let mut cursor_animator =
            Animator::load_new(assets, ResourcePaths::SELECT_CURSOR_ANIMATION);
        cursor_animator.set_state("DEFAULT");
        cursor_animator.set_loop_mode(AnimatorLoopMode::Loop);

        Self {
            camera: Camera::new_ui(game_io),
            background: Background::new_sub_scene(game_io),
            frame: SubSceneFrame::new(game_io)
                .with_top_bar(true)
                .with_arms(true),
            edit_prop,
            server_info,
            button_9patch,
            input_9patch,
            cursor_sprite,
            cursor_animator,
            ui_input_tracker: UiInputTracker::new(),
            ui_layout,
            ui_receiver,
            save_handle: save_handle.unwrap(),
            next_scene: NextScene::None,
        }
    }
}

impl Scene for ServerEditScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn update(&mut self, game_io: &mut GameIO) {
        self.background.update();

        // update cursor
        let focused_index = self.ui_layout.focused_index().unwrap();
        let focused_bounds = self.ui_layout.get_bounds(focused_index).unwrap();

        self.cursor_animator.update();
        self.cursor_animator.apply(&mut self.cursor_sprite);

        let mut cursor_position = focused_bounds.center_left();
        cursor_position -= self.cursor_sprite.size() * 0.5;
        self.cursor_sprite.set_position(cursor_position);

        // input based updates
        if game_io.is_in_transition() {
            return;
        }

        self.ui_input_tracker.update(game_io);
        self.ui_layout.update(game_io, &self.ui_input_tracker);

        let mut leaving = false;

        if let Ok(ui_message) = self.ui_receiver.try_recv() {
            match ui_message {
                UiMessage::NameUpdated(name) => self.server_info.name = name,
                UiMessage::AddressUpdated(address) => self.server_info.address = address,
                UiMessage::Cancel => leaving = true,
                UiMessage::Save => {
                    let global_save = &mut game_io.resource_mut::<Globals>().unwrap().global_save;

                    match self.edit_prop {
                        ServerEditProp::Edit(index) => {
                            global_save.server_list[index] = self.server_info.clone();
                        }
                        ServerEditProp::InsertAfter(index) => {
                            let insert_index = index + 1;
                            let insert_index = insert_index.min(global_save.server_list.len());

                            global_save
                                .server_list
                                .insert(insert_index, self.server_info.clone());
                        }
                    }

                    global_save.save();

                    leaving = true;
                }
            }
        }

        // detect leaving
        let input_util = InputUtil::new(game_io);

        if !self.ui_layout.is_focus_locked() {
            if input_util.was_just_pressed(Input::End) {
                let globals = game_io.resource::<Globals>().unwrap();
                globals.audio.play_sound(&globals.sfx.cursor_move);

                self.ui_layout.set_focused_index(Some(self.save_handle));
            }

            if input_util.was_just_pressed(Input::Cancel) {
                let globals = game_io.resource::<Globals>().unwrap();
                globals.audio.play_sound(&globals.sfx.cursor_cancel);

                leaving = true;
            }
        }

        if leaving {
            let transition = crate::transitions::new_sub_scene_pop(game_io);
            self.next_scene = NextScene::new_pop().with_transition(transition);
        }
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        // draw static pieces
        self.background.draw(game_io, render_pass);

        let scene_title = match self.edit_prop {
            ServerEditProp::Edit(_) => "EDIT SERVER",
            _ => "NEW SERVER",
        };

        self.frame.draw(&mut sprite_queue);
        SceneTitle::new(scene_title).draw(game_io, &mut sprite_queue);

        // draw ui
        self.ui_layout.draw(game_io, &mut sprite_queue);

        if !self.ui_layout.is_focus_locked() {
            // hide the cursor if we can't control it
            sprite_queue.draw_sprite(&self.cursor_sprite);
        }

        render_pass.consume_queue(sprite_queue);
    }
}
