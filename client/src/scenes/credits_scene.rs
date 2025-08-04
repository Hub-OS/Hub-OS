use crate::bindable::SpriteColorMode;
use crate::render::ui::FontName;
use crate::render::ui::TextStyle;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;

const PADDING_TOP: f32 = 16.0;
const NAME_LINE_HEIGHT: f32 = 20.0;

const FADE_IN_TIME: FrameTime = 20;
const FADE_OUT_START_TIME: FrameTime = FADE_IN_TIME + 180;
const HOLD_BLANK_TIME: FrameTime = FADE_OUT_START_TIME + 20;
const PAGE_TIME: FrameTime = HOLD_BLANK_TIME + 10;

const NAMES_PER_PAGE: usize = 5;

const SECTIONS: &[(&str, &[&str])] = &[
    ("credits-heading", &[]),
    // ("credits-music", &["Mondo.sp"]),
    ("credits-sound-design", &["DJRezzed"]),
    ("credits-graphics", &["Konst", "Salad"]),
    ("credits-user-experience", &["Gray Nine", "KayThree"]),
    ("credits-technical-design", &["Mars", "PlayerZero"]),
    ("credits-programmers", &["Dawn Elaine", "Konst"]),
    ("credits-core-team", &["Dawn Elaine", "Mars", "Konst"]),
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
            "JamesKing",
            "JonTheBurger",
            "Keristero",
            "Kuri",
            "kyqurikan",
            "Loui",
            "Mint",
            "Mithalan",
            "Mx. Claris Glennwood",
            "nickblock",
            "OuroYisus",
            "Pion",
            "Salad",
            "Shale",
            "svenevs",
            "theWildSushii",
            "Pheelbert",
            "Poly",
            "Pyredrid",
            "Weenie",
            "Zeek",
        ],
    ),
    (
        "credits-inpsirations",
        &["Mega Man Battle Network", "Open Net Battle"],
    ),
];

pub struct CreditsScene {
    camera: Camera,
    background: Background,
    header_text: String,
    section_index: usize,
    top_name: usize,
    page_time: FrameTime,
    time: FrameTime,
    started: bool,
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
            section_index: 0,
            top_name: 0,
            page_time: 0,
            time: 0,
            started: false,
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

        self.page_time += 1;
        self.time += 1;

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
            self.header_text = globals.translate(header_key);
        } else {
            // out of pages
            self.header_text.clear();

            if self.section_index > SECTIONS.len() {
                // close after a completely blank page
                self.end_scene(game_io);
            }
        }
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        // draw background
        self.background.draw(game_io, render_pass);

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

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
