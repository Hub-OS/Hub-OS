use super::{FontStyle, TextStyle, UiInputTracker, UiNode};
use crate::render::SpriteColorQueue;
use crate::resources::*;
use crate::saves::Config;
use framework::prelude::*;
use std::cell::RefCell;
use std::rc::Rc;

pub struct UiConfigVolume {
    sfx: bool,
    level_text: String,
    locking_focus: bool,
    ui_input_tracker: UiInputTracker,
    config: Rc<RefCell<Config>>,
}

impl UiConfigVolume {
    fn new(sfx: bool, config: Rc<RefCell<Config>>) -> Self {
        let level = if sfx {
            config.borrow().sfx
        } else {
            config.borrow().music
        };

        Self {
            sfx,
            level_text: Self::generate_text(level),
            locking_focus: false,
            ui_input_tracker: UiInputTracker::new(),
            config,
        }
    }

    pub fn new_music(config: Rc<RefCell<Config>>) -> Self {
        Self::new(false, config)
    }

    pub fn new_sfx(config: Rc<RefCell<Config>>) -> Self {
        Self::new(true, config)
    }

    fn generate_text(level: u8) -> String {
        format!("{}/{}", level, MAX_VOLUME)
    }
}

impl UiNode for UiConfigVolume {
    fn draw_bounded(
        &mut self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        bounds: Rect,
    ) {
        let mut text_style = TextStyle::new(game_io, FontStyle::Thick);
        text_style.shadow_color = TEXT_DARK_SHADOW_COLOR;
        text_style.bounds.set_position(bounds.position());

        // draw name
        let name = if self.sfx { "SFX" } else { "Music" };
        text_style.draw(game_io, sprite_queue, name);

        // draw value
        text_style.monospace = true;
        let metrics = text_style.measure(&self.level_text);
        text_style.bounds.x += bounds.width - metrics.size.x - 1.0;

        text_style.draw(game_io, sprite_queue, &self.level_text);
    }

    fn measure_ui_size(&mut self, _: &GameIO) -> Vec2 {
        Vec2::ZERO
    }

    fn is_locking_focus(&self) -> bool {
        self.locking_focus
    }

    fn update(&mut self, game_io: &mut GameIO, _bounds: Rect, focused: bool) {
        self.ui_input_tracker.update(game_io);

        if !focused {
            return;
        }

        let input_util = InputUtil::new(game_io);

        let confirm = input_util.was_just_pressed(Input::Confirm);
        let left = self.ui_input_tracker.is_active(Input::Left);
        let right = self.ui_input_tracker.is_active(Input::Right);

        if !self.locking_focus {
            if left || right || confirm {
                // focus, play sfx, update, and continue
                let globals = game_io.resource::<Globals>().unwrap();
                globals.audio.play_sound(&globals.cursor_select_sfx);

                self.locking_focus = true;
            } else {
                // no focus
                return;
            }
        } else if confirm || input_util.was_just_pressed(Input::Cancel) {
            // unfocus and play sfx
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.cursor_select_sfx);

            self.locking_focus = false;
            return;
        }

        if !self.locking_focus {
            // can't change volume unless the input is selected
            return;
        }

        let up = self.ui_input_tracker.is_active(Input::Up);
        let down = self.ui_input_tracker.is_active(Input::Down);

        // adjusting volume
        let mut config = self.config.borrow_mut();
        let level = if self.sfx {
            &mut config.sfx
        } else {
            &mut config.music
        };

        let original_level = *level;

        // nudge by 1
        if (left || down) && *level > 0 {
            *level -= 1;
        }

        if (right || up) && *level < MAX_VOLUME {
            *level += 1;
        }

        // nudge by 10
        if self.ui_input_tracker.is_active(Input::ShoulderL) {
            if *level >= 10 {
                *level -= 10;
            } else {
                *level = 0;
            }
        }

        if self.ui_input_tracker.is_active(Input::ShoulderR) {
            if *level <= MAX_VOLUME - 10 {
                *level += 10;
            } else {
                *level = MAX_VOLUME;
            }
        }

        if *level == original_level {
            // no change, no need to update anything
            return;
        }

        // update visual
        self.level_text = Self::generate_text(*level);

        // update and test volume
        let volume = if self.sfx {
            config.sfx_volume()
        } else {
            config.music_volume()
        };

        let globals = game_io.resource_mut::<Globals>().unwrap();
        let audio = &mut globals.audio;

        if self.sfx {
            audio.set_sfx_volume(volume);
            audio.play_sound(&globals.cursor_move_sfx);
        } else {
            audio.set_music_volume(volume);
        }
    }
}
