use crate::ResourcePaths;
use crate::battle::PlayerInput;
use crate::render::{Animator, SpriteColorQueue};
use crate::resources::{AssetManager, Globals, RESOLUTION_F};
use crate::saves::DisplayInput;
use framework::prelude::*;
use packets::structures::{Direction, Input};

const ICON_LEN: f32 = 12.0;
const TOP: f32 = 20.0;

const SIMPLIFIED_TESTS: &[(&str, &[Input])] = &[
    ("A", &[Input::UseCard, Input::Confirm]),
    ("B", &[Input::Shoot, Input::Cancel]),
    ("X", &[Input::Special]),
    ("Y", &[Input::FaceLeft, Input::FaceRight]),
    (
        "SHOULDERS",
        &[
            Input::ShoulderL,
            Input::ShoulderR,
            Input::Flee,
            Input::Info,
            Input::EndTurn,
        ],
    ),
    ("SELECT", &[Input::Option2]),
    ("START", &[Input::Option, Input::Pause, Input::End]),
];

const BATTLE_TESTS: &[(&str, Input)] = &[
    ("UP", Input::Up),
    ("DOWN", Input::Down),
    ("LEFT", Input::Left),
    ("RIGHT", Input::Right),
    ("USE_CARD", Input::UseCard),
    ("SHOOT", Input::Shoot),
    ("SPECIAL", Input::Special),
    ("FACE_LEFT", Input::FaceLeft),
    ("FACE_RIGHT", Input::FaceRight),
    ("END_TURN", Input::EndTurn),
    ("OPTION", Input::Option),
    ("PAUSE", Input::Pause),
];

const FULL_TESTS: &[(&str, Input)] = &[
    ("UP", Input::Up),
    ("DOWN", Input::Down),
    ("LEFT", Input::Left),
    ("RIGHT", Input::Right),
    ("USE_CARD", Input::UseCard),
    ("CONFIRM", Input::Confirm),
    ("SHOOT", Input::Shoot),
    ("CANCEL", Input::Cancel),
    ("SPECIAL", Input::Special),
    ("FACE_LEFT", Input::FaceLeft),
    ("FACE_RIGHT", Input::FaceRight),
    ("SHOULDER_L", Input::ShoulderL),
    ("SHOULDER_R", Input::ShoulderR),
    ("FLEE", Input::Flee),
    ("INFO", Input::Info),
    ("END_TURN", Input::EndTurn),
    ("OPTION", Input::Option),
    ("OPTION2", Input::Option2),
    ("END", Input::End),
    ("PAUSE", Input::Pause),
];

pub struct InputDisplay {
    animator: Animator,
    sprites: Vec<Sprite>,
    visible_sprites: usize,
}

impl InputDisplay {
    pub fn new(game_io: &GameIO) -> Self {
        let globals = Globals::from_resources(game_io);

        Self {
            animator: Animator::load_new(&globals.assets, ResourcePaths::BATTLE_INPUTS_ANIMATION),
            sprites: Default::default(),
            visible_sprites: 0,
        }
    }

    fn display_input(&mut self, game_io: &GameIO, globals: &Globals, state: &str) {
        let assets = &globals.assets;

        let sprite = match self.sprites.get_mut(self.visible_sprites) {
            Some(sprite) => sprite,
            None => {
                let sprite = assets.new_sprite(game_io, ResourcePaths::BATTLE_INPUTS);
                self.sprites.push(sprite);
                self.sprites.last_mut().unwrap()
            }
        };

        self.visible_sprites += 1;

        self.animator.set_state(state);
        self.animator.apply(sprite);
    }

    pub fn draw(
        &mut self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        player_input: &PlayerInput,
    ) {
        // clear visible sprites
        self.visible_sprites = 0;

        // resolve sprites
        let globals = Globals::from_resources(game_io);

        match globals.config.display_input {
            DisplayInput::Off => {
                return;
            }
            DisplayInput::Simplified => {
                // simplify direction input
                let mut x = 0;
                let mut y = 0;

                if player_input.is_down(Input::Left) {
                    x -= 1;
                }
                if player_input.is_down(Input::Right) {
                    x += 1;
                }

                if player_input.is_down(Input::Up) {
                    y -= 1;
                }
                if player_input.is_down(Input::Down) {
                    y += 1;
                }

                let direction_state = match Direction::from_i32_vector((x, y)) {
                    Direction::None => None,
                    Direction::Up => Some("UP"),
                    Direction::Left => Some("LEFT"),
                    Direction::Down => Some("DOWN"),
                    Direction::Right => Some("RIGHT"),
                    Direction::UpLeft => Some("UP_LEFT"),
                    Direction::UpRight => Some("UP_RIGHT"),
                    Direction::DownLeft => Some("DOWN_LEFT"),
                    Direction::DownRight => Some("DOWN_RIGHT"),
                };

                if let Some(state) = direction_state {
                    self.display_input(game_io, globals, state);
                }

                // simplify other inputs
                for (state, inputs) in SIMPLIFIED_TESTS {
                    for input in *inputs {
                        if player_input.is_down(*input) {
                            self.display_input(game_io, globals, state);
                            break;
                        }
                    }
                }
            }
            DisplayInput::Battle => {
                for (state, input) in BATTLE_TESTS {
                    if player_input.is_down(*input) {
                        self.display_input(game_io, globals, state);
                    }
                }
            }
            DisplayInput::Full => {
                for (state, input) in FULL_TESTS {
                    if player_input.is_down(*input) {
                        self.display_input(game_io, globals, state);
                    }
                }
            }
        }

        // render sprites
        let mut x = (RESOLUTION_F.x - self.visible_sprites as f32 * ICON_LEN) * 0.5;

        for sprite in &mut self.sprites[..self.visible_sprites] {
            sprite.set_position(Vec2::new(x, TOP));
            sprite_queue.draw_sprite(sprite);
            x += ICON_LEN;
        }
    }
}
