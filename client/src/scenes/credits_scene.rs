use crate::bindable::SpriteColorMode;
use crate::overworld::components::ActorPropertyAnimator;
use crate::overworld::components::MovementAnimator;
use crate::packages::PackageNamespace;
use crate::render::ui::FontName;
use crate::render::ui::TextStyle;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;
use packets::structures::*;
use rand::seq::SliceRandom;
use std::collections::HashMap;

const PADDING_TOP: f32 = 16.0;
const NAME_LINE_HEIGHT: f32 = 20.0;

const FADE_IN_TIME: FrameTime = 20;
const FADE_OUT_START_TIME: FrameTime = FADE_IN_TIME + 180;
const HOLD_BLANK_TIME: FrameTime = FADE_OUT_START_TIME + 20;
const PAGE_TIME: FrameTime = HOLD_BLANK_TIME + 10;

const NAMES_PER_PAGE: usize = 5;

const VARIABLE_PREFIX: &str = "$";

const SECTIONS: &[(&str, &[&str])] = &[
    ("credits-heading", &[]),
    ("credits-music", &["mondo."]),
    ("credits-sound-design", &["DJRezzed"]),
    ("credits-graphics", &["Konst", "Salad"]),
    ("credits-user-experience", &["Gray Nine", "KayThree"]),
    (
        "credits-testers",
        &[
            "DeltaFiend",
            "Jack",
            "KayThree",
            "Kyqurikan",
            "PlayerZero",
            "Tim (Tchomp)",
            "Void.ExE",
        ],
    ),
    (
        "credits-technical-design",
        &["Dawn Elaine", "JamesKing", "Mars"],
    ),
    (
        "credits-programmers",
        &["Dawn Elaine", "JamesKing", "Konst"],
    ),
    ("credits-core-team", &["Dawn Elaine", "Konst", "Mars"]),
    (
        "credits-special-thanks",
        &[
            "Abigail Allbright",
            "Alrysc",
            "cantrem404",
            "CosmicNobab",
            "ChordMankey",
            "Dunstan",
            "Ehab2020",
            "FrozenLake",
            "Gemini",
            "Gwyneth Hestin",
            "JonTheBurger",
            "Keristero",
            "Kuri",
            "Loui",
            "Mint",
            "Mx. Claris Glennwood",
            "Na'Tali",
            "nickblock",
            "OuroYisus",
            "Pion",
            "Pheelbert",
            "Poly",
            "Pyredrid",
            "Shale",
            "svenevs",
            "theWildSushii",
            "Weenie",
            "Zeek",
        ],
    ),
    ("credits-roots", &["Open Net Battle"]),
    (
        "credits-inpsiration",
        &["$credits-mega-man-battle-network", "", "Capcom"],
    ),
    ("", &[""]),
    ("", &["$credits-reach-out-1", "$credits-reach-out-2"]),
];

pub struct CreditsScene {
    camera: Camera,
    background: Background,
    header_text: String,
    translation_cache: HashMap<&'static str, String>,
    section_index: usize,
    top_name: usize,
    page_time: FrameTime,
    time: FrameTime,
    started: bool,
    fun: CreditsFun,
    next_scene: NextScene,
}

impl CreditsScene {
    pub fn new(game_io: &GameIO) -> Self {
        let mut camera = Camera::new(game_io);
        camera.snap(RESOLUTION_F * 0.5);

        let globals = game_io.resource::<Globals>().unwrap();

        Self {
            camera,
            background: Background::new_credits(game_io),
            header_text: globals.translate(SECTIONS[0].0),
            translation_cache: Default::default(),
            section_index: 0,
            top_name: 0,
            page_time: 0,
            time: 0,
            started: false,
            fun: CreditsFun::new(game_io),
            next_scene: NextScene::None,
        }
    }

    fn handle_input(&mut self, game_io: &GameIO) {
        let input_util = InputUtil::new(game_io);

        if input_util.was_just_pressed(Input::Cancel) {
            self.end_scene(game_io);
        }
    }

    fn end_scene(&mut self, game_io: &GameIO) {
        let transition = crate::transitions::new_sub_scene_pop(game_io);
        self.next_scene = NextScene::new_pop().with_transition(transition);
    }

    fn animate(&mut self, game_io: &GameIO) {
        self.page_time += 1;
        self.time += 1;
        self.fun.update(game_io, self.time);

        if self.page_time < PAGE_TIME {
            return;
        }

        // advance page
        self.top_name += NAMES_PER_PAGE;
        self.page_time = 0;

        let total_names_in_section = SECTIONS
            .get(self.section_index)
            .map(|(_, names)| names.len())
            .unwrap_or(0);

        if self.top_name < total_names_in_section {
            return;
        }

        // advance section
        self.section_index += 1;
        self.top_name = 0;

        if let Some((header_key, _)) = SECTIONS.get(self.section_index) {
            let globals = game_io.resource::<Globals>().unwrap();

            self.header_text = if header_key.is_empty() {
                String::new()
            } else {
                globals.translate(header_key)
            };
        } else {
            // out of pages
            self.header_text.clear();

            if self.section_index > SECTIONS.len() {
                // close after a completely blank page
                self.end_scene(game_io);
            }
        }
    }
}

impl Scene for CreditsScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn update(&mut self, game_io: &mut GameIO) {
        self.camera.update();
        self.background.update();

        if !game_io.is_in_transition() {
            self.handle_input(game_io);
            self.started = true
        }

        if !self.started {
            return;
        }

        self.animate(game_io);

        // 2x
        #[cfg(debug_assertions)]
        if InputUtil::new(game_io).is_down(Input::Confirm) {
            self.animate(game_io);
        }
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        // draw background
        self.background.draw(game_io, render_pass);

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        // draw a backdrop to make text more clear
        let globals = game_io.resource::<Globals>().unwrap();
        // let assets = &globals.assets;

        // let mut backdrop = assets.new_sprite(game_io, ResourcePaths::WHITE_PIXEL);
        // backdrop.set_color(Color::BLACK.multiply_alpha(0.75));

        // let backdrop_size = Vec2::new(RESOLUTION_F.x * 0.6, RESOLUTION_F.y * 0.85).floor();
        // backdrop.set_bounds(Rect::from_corners(
        //     Vec2::new(RESOLUTION_F.x * 0.2, PADDING_TOP - 4.0),
        //     Vec2::new(RESOLUTION_F.x * 0.8, RESOLUTION_F.y - PADDING_TOP),
        // ));
        // backdrop.set_position((RESOLUTION_F - backdrop_size) * 0.5);

        // sprite_queue.draw_sprite(&backdrop);

        // draw the fun stuff under credits
        self.fun.draw(&mut sprite_queue);

        // rendering the credits
        let mut text_style = TextStyle::new(game_io, FontName::Thick);
        text_style.shadow_color = TEXT_DARK_SHADOW_COLOR;

        // logic for rendering the header
        text_style.color.a = match self.page_time {
            ..FADE_IN_TIME => {
                inverse_lerp!(0, FADE_IN_TIME, self.page_time)
            }
            FADE_IN_TIME..FADE_OUT_START_TIME => 1.0,
            FADE_OUT_START_TIME..HOLD_BLANK_TIME => {
                inverse_lerp!(HOLD_BLANK_TIME, FADE_OUT_START_TIME, self.page_time)
            }
            HOLD_BLANK_TIME.. => 0.0,
        };
        text_style.shadow_color.a = text_style.color.a;

        // render names
        if let Some((_, names)) = SECTIONS.get(self.section_index) {
            // resolve where to start drawing the names by centering within the available space
            let available_space_top = text_style.line_height() + PADDING_TOP;

            // dynamic bottom padding
            let total_names_on_page = (names.len() - self.top_name).min(NAMES_PER_PAGE);
            let padding_bottom =
                NAME_LINE_HEIGHT * (NAMES_PER_PAGE - total_names_on_page) as f32 * 0.5;

            let available_space = RESOLUTION_F.y - available_space_top - padding_bottom;

            let used_space = total_names_on_page as f32 * NAME_LINE_HEIGHT;
            text_style.bounds.y =
                available_space_top + ((available_space - used_space) * 0.5).floor();

            text_style.font = FontName::Thin;

            for name in names[self.top_name..].iter().take(NAMES_PER_PAGE) {
                let name = if let Some(key) = name.strip_prefix(VARIABLE_PREFIX) {
                    let translated = self
                        .translation_cache
                        .entry(name)
                        .or_insert_with(|| globals.translate(key));

                    translated.as_str()
                } else {
                    name
                };

                let header_size = text_style.measure(name).size;
                text_style.bounds.x = (RESOLUTION_F.x - header_size.x) * 0.5;
                text_style.draw(game_io, &mut sprite_queue, name);

                text_style.bounds.y += NAME_LINE_HEIGHT;
            }

            // avoid fading header the header out if we have more names in this section
            if self.page_time > FADE_IN_TIME && self.top_name + NAMES_PER_PAGE < names.len() {
                text_style.color.a = 1.0;
                text_style.shadow_color.a = 1.0;
            }
        }

        if self.page_time <= FADE_IN_TIME && self.top_name > 0 {
            text_style.color.a = 1.0;
            text_style.shadow_color.a = 1.0;
        }

        text_style.font = FontName::Thick;

        let header_size = text_style.measure(&self.header_text).size;
        text_style.bounds.x = (RESOLUTION_F.x - header_size.x) * 0.5;
        text_style.bounds.y = PADDING_TOP;
        text_style.draw(game_io, &mut sprite_queue, &self.header_text);

        render_pass.consume_queue(sprite_queue);
    }
}

struct CreditsFun {
    entities: hecs::World,
    main_character: hecs::Entity,
    character_pool: Vec<PackageId>,
    used_pool: Vec<PackageId>,
}

impl CreditsFun {
    pub fn new(game_io: &GameIO) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();

        let mut character_pool: Vec<_> = globals
            .player_packages
            .package_ids(PackageNamespace::Local)
            .filter(|id| **id != globals.global_save.selected_character)
            .cloned()
            .collect();
        character_pool.shuffle(&mut rand::rng());

        let mut entities = hecs::World::new();
        let main_character = Self::spawn_character(
            game_io,
            &mut entities,
            globals.global_save.selected_character.clone(),
        );

        Self {
            entities,
            main_character,
            character_pool,
            used_pool: Vec::new(),
        }
    }

    fn spawn_character(
        game_io: &GameIO,
        entities: &mut hecs::World,
        package_id: PackageId,
    ) -> hecs::Entity {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let package = globals
            .player_packages
            .package(PackageNamespace::Local, &package_id)
            .unwrap();

        entities.spawn((
            assets.new_sprite(game_io, &package.overworld_paths.texture),
            Animator::load_new(assets, &package.overworld_paths.animation),
            MovementAnimator::new(),
            Vec3::new(-5000.0, 0.0, 0.0), // place somewhere offscreen
            Direction::Down,
            package_id,
        ))
    }

    fn animate(&mut self, game_io: &GameIO, entity: hecs::Entity, key_frames: Vec<ActorKeyFrame>) {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let mut property_animator = ActorPropertyAnimator::new();

        for keyframe in key_frames {
            property_animator.add_key_frame(keyframe);
        }

        if let Err(err) = self.entities.insert_one(entity, property_animator) {
            log::error!("{err}");
        }

        ActorPropertyAnimator::start(game_io, assets, &mut self.entities, entity);
    }

    fn update(&mut self, game_io: &GameIO, time: FrameTime) {
        self.script(game_io, time);
        self.systems(game_io);
    }

    fn systems(&mut self, game_io: &GameIO) {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        crate::overworld::system_update_animation(&mut self.entities);
        ActorPropertyAnimator::update(game_io, assets, &mut self.entities);
        crate::overworld::system_movement_animation(&mut self.entities);
        crate::overworld::system_apply_animation(&mut self.entities);

        // clean up
        let mut pending_despawn = Vec::new();

        for (id, _) in self
            .entities
            .query_mut::<hecs::Without<(), &ActorPropertyAnimator>>()
        {
            if id != self.main_character {
                pending_despawn.push(id);
            }
        }

        for entity in pending_despawn {
            if let Ok(package_id) = self.entities.remove_one(entity) {
                self.used_pool.push(package_id)
            }

            let _ = self.entities.despawn(entity);
        }
    }

    fn draw(&mut self, sprite_queue: &mut SpriteColorQueue) {
        // update positions
        for (_, (sprite, position)) in self.entities.query_mut::<(&mut Sprite, &Vec3)>() {
            let position = position.xy() + Vec2::new(0.0, position.z);
            sprite.set_position(position);
        }

        // sort and render
        let mut sprites: Vec<_> = self
            .entities
            .query_mut::<(&Sprite, &Vec3)>()
            .into_iter()
            .map(|(_, (sprite, position))| (sprite, position.y as i32))
            .collect();

        sprites.sort_by_key(|(_, y)| *y);

        for (sprite, _) in sprites {
            sprite_queue.draw_sprite(sprite);
        }
    }

    fn pull_character(&mut self) -> Option<PackageId> {
        if self.character_pool.is_empty() {
            std::mem::swap(&mut self.character_pool, &mut self.used_pool);
            self.character_pool.shuffle(&mut rand::rng());
        }

        self.character_pool.pop()
    }

    fn spawn_random_character_or_fallback(&mut self, game_io: &GameIO) -> hecs::Entity {
        self.pull_character()
            .map(|package_id| Self::spawn_character(game_io, &mut self.entities, package_id))
            .unwrap_or(self.main_character)
    }

    fn animate_simple_movement(
        &mut self,
        game_io: &GameIO,
        entity: hecs::Entity,
        start: Vec2,
        offsets: &[(Vec2, Direction, f32)],
    ) {
        let mut key_frames = Vec::with_capacity(offsets.len() + 1);

        key_frames.push(ActorKeyFrame {
            property_steps: vec![
                (ActorProperty::X(start.x), Ease::Floor),
                (ActorProperty::Y(start.y), Ease::Floor),
                (ActorProperty::Z(0.0), Ease::Floor),
            ],
            duration: 0.0,
        });

        let mut position = start;
        let mut direction = Direction::Down;

        for (offset, specified_direction, duration) in offsets {
            position += *offset;

            let new_direction = if specified_direction.is_none() {
                Direction::from_offset((*offset).into()).rotate_cc()
            } else {
                *specified_direction
            };

            if new_direction != Direction::None {
                direction = new_direction;
            }

            key_frames.push(ActorKeyFrame {
                property_steps: vec![
                    (ActorProperty::X(position.x), Ease::Linear),
                    (ActorProperty::Y(position.y), Ease::Linear),
                    (ActorProperty::Direction(direction), Ease::Floor),
                ],
                duration: *duration,
            });
        }

        self.animate(game_io, entity, key_frames);
    }

    fn script(&mut self, game_io: &GameIO, time: FrameTime) {
        const OFFSCREEN_PADDING: f32 = 64.0;
        const OFFSTAGE_LEFT: f32 = -OFFSCREEN_PADDING;
        const OFFSTAGE_RIGHT: f32 = RESOLUTION_F.x + OFFSCREEN_PADDING;
        const OFFSTAGE_UP: f32 = -OFFSCREEN_PADDING;
        const OFFSTAGE_DOWN: f32 = RESOLUTION_F.y + OFFSCREEN_PADDING;
        const STAGE_WIDTH: f32 = OFFSTAGE_RIGHT - OFFSTAGE_LEFT;
        const STAGE_HEIGHT: f32 = OFFSTAGE_DOWN - OFFSTAGE_UP;
        const STAGE_FLOOR: f32 = RESOLUTION_F.y - 8.0;

        // moving down right by default
        const DIAGONAL_STEP: Vec2 = Vec2::new(32.0, 16.0);
        const DIAGONAL_TIME: f32 = 1.0;
        // moving right by default
        const HORIZONTAL_STEP: Vec2 = Vec2::new(64.0, 0.0);
        const HORIZONTAL_TIME: f32 = 1.25;
        // moving down by default
        const VERTICAL_STEP: Vec2 = Vec2::new(0.0, 32.0);
        const VERTICAL_TIME: f32 = 0.75;

        const fn count_page_frames(pages: usize) -> FrameTime {
            PAGE_TIME * pages as FrameTime + 5
        }

        const fn section_end_frame(section_index: usize) -> FrameTime {
            let mut pages = 0;
            let mut i = 0;

            loop {
                let section = SECTIONS[i];
                let section_page_count = section.1.len().div_ceil(NAMES_PER_PAGE);

                if section_page_count == 0 {
                    pages += 1;
                } else {
                    pages += section_page_count;
                }

                if i == section_index {
                    break;
                }

                i += 1;
            }

            count_page_frames(pages)
        }

        const fn to_seconds(time: FrameTime) -> f32 {
            time as f32 / 60.0
        }

        const FIRST_SWITCH: FrameTime = section_end_frame(4) - FADE_IN_TIME * 2 / 3;
        const SECOND_SWITCH: FrameTime = section_end_frame(8) - FADE_IN_TIME / 2;
        const THIRD_SWITCH: FrameTime = section_end_frame(9) - FADE_IN_TIME / 2;

        match time {
            1 => {
                // enter from the left, watch the credits, exit to the right
                let movement_1 = STAGE_WIDTH * 0.55;
                let movement_2 = STAGE_WIDTH - movement_1;

                let enter_duration = movement_1 / HORIZONTAL_STEP.x * HORIZONTAL_TIME;
                let wait_duration = to_seconds(FIRST_SWITCH) - enter_duration;

                self.animate_simple_movement(
                    game_io,
                    self.main_character,
                    Vec2::new(OFFSTAGE_LEFT, STAGE_FLOOR),
                    &[
                        (Vec2::new(movement_1, 0.0), Direction::None, enter_duration),
                        (Vec2::ZERO, Direction::Up, wait_duration),
                        (
                            Vec2::new(movement_2, 0.0),
                            Direction::None,
                            movement_2 / HORIZONTAL_STEP.x * HORIZONTAL_TIME,
                        ),
                    ],
                );
            }
            FIRST_SWITCH => {
                // enter from the left diagonally, exit up-right then up
                let package_id = self.pull_character();

                if let Some(package_id) = package_id {
                    let entity = Self::spawn_character(game_io, &mut self.entities, package_id);

                    let steps_1 = 3.0;
                    let steps_2 = 4.0;

                    let enter_duration = DIAGONAL_TIME * steps_1;
                    let wait_duration =
                        to_seconds(SECOND_SWITCH) - enter_duration - to_seconds(time);

                    self.animate_simple_movement(
                        game_io,
                        entity,
                        Vec2::new(OFFSTAGE_LEFT, RESOLUTION_F.y * 0.25),
                        &[
                            (DIAGONAL_STEP * steps_1, Direction::None, enter_duration),
                            (Vec2::ZERO, Direction::None, wait_duration),
                            (
                                VERTICAL_STEP * Vec2::new(1.0, -1.0) * steps_2,
                                Direction::None,
                                VERTICAL_TIME * steps_2,
                            ),
                        ],
                    );
                }
            }
            SECOND_SWITCH => {
                // send a pair from the right if possible
                let shared_steps = 1.5;
                let shart_start = Vec2::new(OFFSTAGE_RIGHT, RESOLUTION_F.y * 0.75);

                let start_time = to_seconds(time);
                let switch_time = to_seconds(THIRD_SWITCH);

                let enter_duration = HORIZONTAL_TIME * shared_steps;
                let wait_duration = switch_time - enter_duration - start_time;

                let entity_a = self.spawn_random_character_or_fallback(game_io);
                self.animate_simple_movement(
                    game_io,
                    entity_a,
                    shart_start + Vec2::new(0.0, 0.0),
                    &[
                        (
                            HORIZONTAL_STEP * -shared_steps,
                            Direction::None,
                            enter_duration,
                        ),
                        (Vec2::ZERO, Direction::None, wait_duration),
                        (
                            HORIZONTAL_STEP * shared_steps,
                            Direction::None,
                            HORIZONTAL_TIME * shared_steps,
                        ),
                    ],
                );

                let friend_package_package_b = self.pull_character();

                if let Some(package_id) = friend_package_package_b {
                    let entity = Self::spawn_character(game_io, &mut self.entities, package_id);

                    let return_steps = shared_steps;

                    // delay to not move awkwardly in sync with the other navi
                    let delay_duration = 0.2;
                    // how long it takes to enter or exit once
                    let enter_duration = HORIZONTAL_TIME * shared_steps;
                    // watching
                    let wait_1_duration =
                        to_seconds(count_page_frames(4)) - delay_duration - enter_duration;
                    // waiting outside
                    let wait_2_duration = 2.5;
                    // waiting for the first navi to leave
                    let wait_3_duration = switch_time
                        - wait_2_duration
                        - wait_1_duration
                        - delay_duration
                        - enter_duration * 3.0 // entered, exited, and re-entered
                        - start_time
                        + 0.5;

                    self.animate_simple_movement(
                        game_io,
                        entity,
                        shart_start + Vec2::new(16.0, -16.0),
                        &[
                            (Vec2::ZERO, Direction::None, delay_duration),
                            (
                                HORIZONTAL_STEP * -shared_steps,
                                Direction::None,
                                enter_duration,
                            ),
                            (Vec2::ZERO, Direction::None, wait_1_duration),
                            (
                                HORIZONTAL_STEP * shared_steps,
                                Direction::None,
                                enter_duration,
                            ),
                            (Vec2::ZERO, Direction::None, wait_2_duration),
                            (
                                HORIZONTAL_STEP * -return_steps,
                                Direction::None,
                                enter_duration,
                            ),
                            (Vec2::ZERO, Direction::DownLeft, wait_3_duration),
                            (
                                HORIZONTAL_STEP * return_steps,
                                Direction::None,
                                enter_duration,
                            ),
                        ],
                    );
                }
            }
            _ => {}
        }
    }
}
