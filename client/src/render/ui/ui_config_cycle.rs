use super::ui_config_cycle_arrows::UiConfigCycleArrows;
use super::{FontName, TextStyle, UiNode};
use crate::render::SpriteColorQueue;
use crate::resources::*;
use crate::saves::Config;
use framework::prelude::*;
use std::cell::{RefCell, RefMut};
use std::rc::Rc;

struct LockedState {
    original_selection: usize,
    arrows: UiConfigCycleArrows,
}

pub struct UiConfigCycle<T> {
    label: String,
    selection: usize,
    options: Vec<(String, T)>,
    config: Rc<RefCell<Config>>,
    callback: Box<dyn Fn(&mut GameIO, RefMut<Config>, T, bool)>,
    locked_state: Option<Box<LockedState>>,
}

impl<T: Copy + PartialEq> UiConfigCycle<T> {
    pub fn new(
        game_io: &GameIO,
        label_translation_key: &str,
        value: T,
        config: Rc<RefCell<Config>>,
        options: &[(&str, T)],
        callback: impl Fn(&mut GameIO, RefMut<Config>, T, bool) + 'static,
    ) -> Self {
        let globals = Globals::from_resources(game_io);

        Self {
            label: globals.translate(label_translation_key),
            selection: options
                .iter()
                .position(|(_, v)| *v == value)
                .unwrap_or_default(),
            options: options
                .iter()
                .map(|(name, value)| (globals.translate(name), *value))
                .collect(),
            config,
            callback: Box::new(callback),
            locked_state: None,
        }
    }
}

impl<T: Copy> UiNode for UiConfigCycle<T> {
    fn draw_bounded(
        &mut self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        bounds: Rect,
    ) {
        let mut text_style = TextStyle::new(game_io, FontName::Thick);
        text_style.shadow_color = TEXT_DARK_SHADOW_COLOR;
        text_style.bounds.set_position(bounds.position());

        // draw name
        text_style.draw(game_io, sprite_queue, &self.label);

        // draw value
        let text = &self.options[self.selection].0;
        let metrics = text_style.measure(text);
        text_style.bounds.x += bounds.width - metrics.size.x - 1.0;

        text_style.draw(game_io, sprite_queue, text);

        if let Some(locked_state) = &mut self.locked_state {
            let mut bounds = bounds;
            bounds.height = metrics.size.y;
            locked_state.arrows.draw(sprite_queue, bounds);
        }
    }

    fn measure_ui_size(&mut self, _: &GameIO) -> Vec2 {
        Vec2::ZERO
    }

    fn is_locking_focus(&self) -> bool {
        self.locked_state.is_some()
    }

    fn update(&mut self, game_io: &mut GameIO, _bounds: Rect, focused: bool) {
        if !focused {
            return;
        }

        let input_util = InputUtil::new(game_io);
        let left = input_util.was_just_pressed(Input::Left);
        let right = input_util.was_just_pressed(Input::Right);
        let confirm = input_util.was_just_pressed(Input::Confirm);

        if !self.is_locking_focus() {
            if confirm {
                // focus, play sfx, update, and continue
                let globals = Globals::from_resources(game_io);
                globals.audio.play_sound(&globals.sfx.cursor_select);

                self.locked_state = Some(Box::new(LockedState {
                    original_selection: self.selection,
                    arrows: UiConfigCycleArrows::new(game_io),
                }));
            }
        } else if confirm {
            // unfocus and play sfx
            let globals = Globals::from_resources(game_io);
            globals.audio.play_sound(&globals.sfx.cursor_select);

            self.locked_state = None;
            let value = self.options[self.selection].1;
            (self.callback)(game_io, self.config.borrow_mut(), value, true);
        } else if input_util.was_just_pressed(Input::Cancel) {
            // unfocus, undo, and play sfx
            let globals = Globals::from_resources(game_io);
            globals.audio.play_sound(&globals.sfx.cursor_cancel);

            if let Some(locked_state) = self.locked_state.take() {
                self.selection = locked_state.original_selection;
                let value = self.options[self.selection].1;
                (self.callback)(game_io, self.config.borrow_mut(), value, true);
            }
        }

        // update arrows, also tests if we're locking input
        let Some(locked_state) = &mut self.locked_state else {
            return;
        };

        locked_state.arrows.update();

        // test input
        if !left && !right {
            return;
        }

        let original_selection = self.selection;

        if left {
            if self.selection == 0 {
                self.selection = self.options.len() - 1
            } else {
                self.selection -= 1;
            }
        }

        if right {
            self.selection += 1;
            self.selection = (self.selection)
                .checked_rem(self.options.len())
                .unwrap_or_default();
        }

        if self.selection == original_selection {
            return;
        }

        let value = self.options[self.selection].1;
        (self.callback)(game_io, self.config.borrow_mut(), value, false);

        let globals = Globals::from_resources(game_io);
        globals.audio.play_sound(&globals.sfx.cursor_move);
    }
}
