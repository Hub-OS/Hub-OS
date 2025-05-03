use super::{BlocksScene, CharacterSelectScene, ManageSwitchDriveScene};
use crate::battle::{Player, PlayerFallbackResources};
use crate::bindable::SpriteColorMode;
use crate::packages::PlayerPackage;
use crate::render::ui::{
    ElementSprite, FontName, PageTracker, PlayerHealthUi, SceneTitle, ScrollableList,
    SubSceneFrame, Text, Textbox, TextboxCharacterNavigation, UiInputTracker, UiNode,
};
use crate::render::{Animator, AnimatorLoopMode, Background, Camera, SpriteColorQueue};
use crate::resources::*;
use framework::prelude::*;
use std::sync::Arc;

enum Event {
    BlockCustomization,
    CharacterSelect,
    EquipDrives,
}

pub struct CharacterScene {
    camera: Camera,
    background: Background,
    frame: SubSceneFrame,
    textbox: Textbox,
    event_sender: flume::Sender<Event>,
    event_receiver: flume::Receiver<Event>,
    page_tracker: PageTracker,
    pages: Vec<StatusPage>,
    input_tracker: UiInputTracker,
    next_scene: NextScene,
}

impl CharacterScene {
    pub fn new(game_io: &GameIO) -> Box<Self> {
        let (event_sender, event_receiver) = flume::unbounded();

        Box::new(Self {
            camera: Camera::new_ui(game_io),
            background: Background::new_sub_scene(game_io),
            frame: SubSceneFrame::new(game_io).with_top_bar(true),
            textbox: Textbox::new_navigation(game_io).begin_open(),
            event_sender,
            event_receiver,
            page_tracker: PageTracker::new(game_io, 2),
            pages: Vec::new(),
            input_tracker: UiInputTracker::new(),
            next_scene: NextScene::None,
        })
    }

    fn load_pages(&mut self, game_io: &GameIO) {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let mut ui_animator = Animator::load_new(assets, ResourcePaths::CHARACTER_UI_ANIMATION);
        let page_sprite = assets.new_sprite(game_io, ResourcePaths::CHARACTER_UI);

        self.pages.clear();

        let data = StatusData::new(game_io);

        for (i, state) in ["PAGE_1", "PAGE_2"].into_iter().enumerate() {
            ui_animator.set_state(state);
            self.pages.push(StatusPage::new(
                game_io,
                &ui_animator,
                page_sprite.clone(),
                &data,
            ));

            if let Some(point) = ui_animator.point("PAGE_ARROWS") {
                self.page_tracker.set_page_arrow_offset(i - 1, point)
            }
        }
    }

    fn reload_textbox(&mut self, game_io: &GameIO) {
        let event_sender = self.event_sender.clone();
        let interface = TextboxCharacterNavigation::new(move |i| {
            let event = match i {
                0 => Event::BlockCustomization,
                1 => Event::EquipDrives,
                _ => Event::CharacterSelect,
            };

            event_sender.send(event).unwrap();
        });

        self.textbox = Textbox::new_navigation(game_io)
            .begin_open()
            .with_interface(interface);
        self.textbox.skip_animation(game_io);
    }

    fn handle_input(&mut self, game_io: &GameIO) {
        self.input_tracker.update(game_io);
        self.page_tracker.handle_input(game_io);

        if !self.page_tracker.animating() {
            let page = &mut self.pages[self.page_tracker.active_page()];
            page.handle_input(game_io, &self.input_tracker);
        }

        let input_util = InputUtil::new(game_io);

        if input_util.was_just_pressed(Input::Cancel) {
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_cancel);

            let transition = crate::transitions::new_scene_pop(game_io);
            self.next_scene = NextScene::new_pop().with_transition(transition);
        }
    }
}

impl Scene for CharacterScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn enter(&mut self, game_io: &mut GameIO) {
        self.load_pages(game_io);
        self.reload_textbox(game_io);
    }

    fn update(&mut self, game_io: &mut GameIO) {
        self.background.update();
        self.textbox.update(game_io);
        self.page_tracker.update();

        for page in &mut self.pages {
            page.update();
        }

        while let Ok(event) = self.event_receiver.try_recv() {
            match event {
                Event::BlockCustomization => {
                    let transition = crate::transitions::new_sub_scene(game_io);
                    self.next_scene =
                        NextScene::new_push(BlocksScene::new(game_io)).with_transition(transition)
                }
                Event::CharacterSelect => {
                    let transition = crate::transitions::new_sub_scene(game_io);
                    self.next_scene = NextScene::new_push(CharacterSelectScene::new(game_io))
                        .with_transition(transition)
                }
                Event::EquipDrives => {
                    let transition = crate::transitions::new_sub_scene(game_io);
                    self.next_scene = NextScene::new_push(ManageSwitchDriveScene::new(game_io))
                        .with_transition(transition)
                }
            }
        }

        if !game_io.is_in_transition() {
            self.handle_input(game_io);
        }
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        self.background.draw(game_io, render_pass);

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        // draw page
        for (index, offset) in self.page_tracker.visible_pages() {
            self.pages[index].draw(game_io, &mut sprite_queue, offset);
        }

        self.page_tracker.draw_page_arrows(&mut sprite_queue);

        // draw title
        self.frame.draw(&mut sprite_queue);
        SceneTitle::new("STATUS").draw(game_io, &mut sprite_queue);

        // draw textbox
        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}

struct StatusData<'a> {
    player_package: &'a PlayerPackage,
    visible_augments: Vec<String>,
    health: i32,
    charge_level: i8,
    attack_level: i8,
    rapid_level: i8,
    deck_restrictions: DeckRestrictions,
}

impl<'a> StatusData<'a> {
    fn new(game_io: &'a GameIO) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let global_save = &globals.global_save;
        let player_package = global_save.player_package(game_io).unwrap();

        // resolve changes from augments
        let mut health = player_package.health;
        let mut attack_level = 1;
        let mut rapid_level = 1;
        let mut charge_level = 1;

        let augments: Vec<_> = global_save.valid_augments(game_io).collect();

        for &(package, level) in &augments {
            health += package.health_boost * level as i32;
            attack_level += package.attack_boost * level as i8;
            rapid_level += package.rapid_boost * level as i8;
            charge_level += package.charge_boost * level as i8;
        }

        // resolve visible augments
        let visible_augments = augments
            .iter()
            .filter(|(augment, _)| augment.has_shape)
            .flat_map(|(augment, level)| std::iter::repeat_n(augment.name.clone(), *level))
            .collect();

        // resolve deck restrictions
        let mut deck_restrictions = globals.restrictions.base_deck_restrictions();
        deck_restrictions.apply_augments(augments.into_iter());

        Self {
            player_package,
            visible_augments,
            health,
            attack_level: attack_level.clamp(1, 5),
            rapid_level: rapid_level.clamp(1, 5),
            charge_level: charge_level.clamp(1, 5),
            deck_restrictions,
        }
    }
}

struct StatusPage {
    text: Vec<Text>,
    sprites: Vec<(Sprite, Option<Arc<Texture>>)>, // (sprite, palette)
    animators: Vec<(Animator, usize)>,
    lists: Vec<ScrollableList>,
    player_health_ui: Vec<PlayerHealthUi>,
}

impl StatusPage {
    fn new(
        game_io: &GameIO,
        ui_animator: &Animator,
        mut page_sprite: Sprite,
        data: &StatusData,
    ) -> Self {
        let mut page = Self {
            text: Vec::new(),
            sprites: Vec::new(),
            animators: Vec::new(),
            lists: Vec::new(),
            player_health_ui: Vec::new(),
        };

        ui_animator.apply(&mut page_sprite);
        page.sprites.push((page_sprite, None));

        if let Some(point) = ui_animator.point("ELEMENT") {
            let mut sprite = ElementSprite::new(game_io, data.player_package.element);
            sprite.set_position(point);
            page.sprites.push((sprite, None));
        }

        if let Some(point) = ui_animator.point("HEALTH") {
            let mut player_health_ui = PlayerHealthUi::new(game_io);
            player_health_ui.snap_health(data.health);
            player_health_ui.set_position(point);

            page.player_health_ui.push(player_health_ui);
        }

        if let Some(point) = ui_animator.point("PLAYER") {
            let player_resources = PlayerFallbackResources::resolve(game_io, data.player_package);

            let globals = game_io.resource::<Globals>().unwrap();

            let texture_path = player_resources.texture_path;
            let mut sprite = globals.assets.new_sprite(game_io, &texture_path);
            sprite.set_position(point);

            let mut animator = player_resources.animator;
            animator.set_state(Player::IDLE_STATE);
            animator.set_loop_mode(AnimatorLoopMode::Loop);
            animator.apply(&mut sprite);

            let palette = player_resources
                .palette_path
                .map(|path| globals.assets.texture(game_io, &path));

            let sprite_index = page.sprites.len();
            page.sprites.push((sprite, palette));

            page.animators.push((animator, sprite_index));
        }

        let stat_text_associations = [
            ("ATTACK_TEXT", "Attack LV ", data.attack_level),
            ("RAPID_TEXT", "Rapid  LV ", data.rapid_level),
            ("CHARGE_TEXT", "Charge LV ", data.charge_level),
        ];

        for (label, prefix, value) in stat_text_associations {
            if let Some(point) = ui_animator.point(label) {
                let mut text = Text::new_monospace(game_io, FontName::Thick);
                text.style.shadow_color = TEXT_DARK_SHADOW_COLOR;
                text.style.letter_spacing = 2.0;
                text.style.bounds.set_position(point);

                text.text = format!("{prefix}{value}");

                page.text.push(text);
            }
        }

        if let Some(point) = ui_animator.point("POWER_CHARGE_TEXT") {
            let mut text = Text::new(game_io, FontName::Thick);
            text.style.shadow_color = TEXT_DARK_SHADOW_COLOR;
            text.style.letter_spacing = 2.0;
            text.style.bounds.set_position(point);

            text.text = String::from("PwrCharge S");

            page.text.push(text);
        }

        if let Some(start_point) = ui_animator.point("DECK_START") {
            let end_point = ui_animator.point_or_zero("DECK_END");
            let bounds = Rect::from_corners(start_point, end_point);

            let list = ScrollableList::new(game_io, bounds, 15.0)
                .with_label_str("FOLDER")
                .with_focus(false)
                .with_children(vec![
                    Box::new(
                        Text::new_monospace(game_io, FontName::Thin)
                            .with_string(format!(
                                "MegaLim {:>2}",
                                data.deck_restrictions.mega_limit
                            ))
                            .with_shadow_color(TEXT_DARK_SHADOW_COLOR),
                    ),
                    Box::new(
                        Text::new_monospace(game_io, FontName::Thin)
                            .with_string(format!(
                                "GigaLim {:>2}",
                                data.deck_restrictions.giga_limit
                            ))
                            .with_shadow_color(TEXT_DARK_SHADOW_COLOR),
                    ),
                ]);

            page.lists.push(list);
        }

        if let Some(start_point) = ui_animator.point("BLOCKS_START") {
            let end_point = ui_animator.point_or_zero("BLOCKS_END");
            let bounds = Rect::from_corners(start_point, end_point);

            let list = ScrollableList::new(game_io, bounds, 15.0)
                .with_label_str("AUGMENTS")
                .with_focus(false)
                .with_children(
                    data.visible_augments
                        .iter()
                        .map(|name| -> Box<dyn UiNode> {
                            Box::new(
                                Text::new(game_io, FontName::Thin)
                                    .with_shadow_color(TEXT_DARK_SHADOW_COLOR)
                                    .with_str(name),
                            )
                        })
                        .collect(),
                );

            page.lists.push(list);
        }

        page
    }

    fn handle_input(&mut self, game_io: &GameIO, input_tracker: &UiInputTracker) {
        let mut scrolled = false;

        for list in &mut self.lists {
            let prev_index = list.selected_index();

            if input_tracker.pulsed(Input::ShoulderL) {
                list.page_up();
            }

            if input_tracker.pulsed(Input::ShoulderR) {
                list.page_down();
            }

            scrolled |= list.selected_index() != prev_index;
        }

        if scrolled {
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_move);
        }
    }

    fn update(&mut self) {
        for (animator, i) in &mut self.animators {
            animator.update();
            animator.apply(&mut self.sprites[*i].0);
        }
    }

    fn draw(&mut self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue, offset_x: f32) {
        let offset = Vec2::new(offset_x, 0.0);

        for player_health_ui in &mut self.player_health_ui {
            let position = player_health_ui.position();
            player_health_ui.set_position(position + offset);

            player_health_ui.draw(game_io, sprite_queue);

            player_health_ui.set_position(position);
        }

        for (sprite, palette) in &mut self.sprites {
            let position = sprite.position();
            sprite.set_position(position + offset);

            sprite_queue.set_palette(palette.clone());
            sprite_queue.draw_sprite(sprite);

            sprite.set_position(position);
        }

        sprite_queue.set_palette(None);

        for text in &mut self.text {
            let bounds = text.style.bounds;
            text.style.bounds = bounds + offset;

            text.draw(game_io, sprite_queue);

            text.style.bounds = bounds;
        }

        for list in &mut self.lists {
            let bounds = list.bounds();
            list.set_bounds(bounds + offset);

            list.draw(game_io, sprite_queue);

            list.set_bounds(bounds);
        }
    }
}
