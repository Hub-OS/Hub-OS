use crate::render::ui::{FontName, TextStyle, UiInputTracker};
use crate::render::{Camera, SpriteColorQueue};
use crate::resources::{AssetManager, Globals, InputUtil, RESOLUTION_F};
use crate::{ResourcePaths, transitions};
use framework::prelude::*;
use packets::structures::Input;

const FONTS: &[FontName] = &[
    FontName::Thick,
    FontName::Thin,
    FontName::Code,
    FontName::Navigation,
    FontName::Context,
    FontName::Micro,
];

const SETS: &[&str] = &[
    "\
ABCDEFGHIJKLMNOPQRSTUVWXYZ
abcdefghijklmnopqrstuvwxyz
A ÁÉÍÓÚ ÄÖÜ À
a áéíóú äöü à
A ÂÊÎÔÛ ÃẼÕ
a âêîôû ãẽõ
C ÇçÑñ",
    "\
A あいうえお はひふへほ ぁぃぅぇぉ
A かきくけこ ばびぶべぼ っ
A がぎぐげご ぱぴぷぺぽ ゃゅょ
A さしすせそ まみむめも
A ざじずぜぞ やゆよ
A たちつてと らりるれろ
A だぢづでど わをんゔ
A なにぬねの",
    "\
A アイウエオ ハヒフヘホ ァィゥェォ
A カキクケコ バビブベボ ッ
A ガギグゲゴ パピプペポ ャュョ
A サシスセソ マミムメモ
A ザジズゼゾ ヤユヨ
A タチツテト ラリルレロ
A ダヂヅデド ワヲヴー
A ナニヌネノ",
];

pub struct FontTestScene {
    camera: Camera,
    font_index: usize,
    set_index: usize,
    monospace: bool,
    input_tracker: UiInputTracker,
    next_scene: NextScene,
}

impl FontTestScene {
    pub fn new(game_io: &GameIO) -> Self {
        let mut camera = Camera::new(game_io);
        camera.snap(RESOLUTION_F * 0.5);

        Self {
            camera,
            font_index: 0,
            set_index: 0,
            monospace: false,
            input_tracker: UiInputTracker::new(),
            next_scene: NextScene::None,
        }
    }
}

impl Scene for FontTestScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn update(&mut self, game_io: &mut GameIO) {
        if game_io.is_in_transition() {
            return;
        }

        self.input_tracker.update(game_io);

        if InputUtil::new(game_io).was_just_pressed(Input::Cancel) {
            self.next_scene =
                NextScene::new_pop().with_transition(transitions::new_sub_scene_pop(game_io));
        }
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        let mut sprite_queue = SpriteColorQueue::new(game_io, &self.camera, Default::default());

        // handle input here to keep it simple
        if self.input_tracker.pulsed(Input::Up) {
            self.font_index = self.font_index.checked_sub(1).unwrap_or(FONTS.len() - 1)
        }

        if self.input_tracker.pulsed(Input::Down) {
            self.font_index += 1;
            self.font_index %= FONTS.len()
        }

        if self.input_tracker.pulsed(Input::Left) {
            self.set_index = self.set_index.checked_sub(1).unwrap_or(SETS.len() - 1)
        }

        if self.input_tracker.pulsed(Input::Right) {
            self.set_index += 1;
            self.set_index %= SETS.len()
        }

        self.monospace ^= self.input_tracker.pulsed(Input::Confirm);

        // resolve test
        let font = &FONTS[self.font_index];
        let text = SETS[self.set_index];

        let mut text_style = TextStyle::new(game_io, font.clone());
        text_style.monospace = self.monospace;
        text_style.line_spacing = 2.0;

        // render lines
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;
        let mut line_sprite = assets.new_sprite(game_io, ResourcePaths::PIXEL);

        let line_height = text_style.line_height();

        for (i, line) in text.split("\n").enumerate() {
            // start with baseline
            line_sprite.set_color(Color::BLUE);

            let mut position = Vec2::new(0.0, (i + 1) as f32 * line_height - 2.0);
            line_sprite.set_position(position);

            let width = text_style.measure(line).size.x;
            line_sprite.set_width(width);

            sprite_queue.draw_sprite(&line_sprite);

            // render the top line
            line_sprite.set_color(Color::WHITE.multiply_color(0.3));
            position.y += -line_height + text_style.line_spacing + 1.0;
            line_sprite.set_position(position);

            sprite_queue.draw_sprite(&line_sprite);
        }

        // render text
        text_style.bounds.set_position(Vec2::new(0.0, 1.0));
        text_style.draw(game_io, &mut sprite_queue, text);

        // render font name
        let font_str = font.as_str();
        let font_str_size = text_style.measure(font_str).size;
        let font_str_pos = Vec2::new(0.0, RESOLUTION_F.y - font_str_size.y - 1.0);
        text_style.bounds.set_position(font_str_pos);
        text_style.draw(game_io, &mut sprite_queue, font_str);

        // render monospace option
        let monospace_str = "MONOSPACE";
        let monospace_str_size = text_style.measure(monospace_str).size;
        let monospace_str_pos = RESOLUTION_F - monospace_str_size - 1.0;
        text_style.bounds.set_position(monospace_str_pos);

        if !self.monospace {
            text_style.color = Color::WHITE.multiply_alpha(0.6);
        }

        text_style.draw(game_io, &mut sprite_queue, monospace_str);

        render_pass.consume_queue(sprite_queue);
    }
}
