use super::{FontStyle, OverflowTextScroller, TextStyle, UiNode};
use crate::render::SpriteColorQueue;
use crate::resources::*;
use crate::saves::Config;
use framework::prelude::*;
use itertools::Itertools;
use std::cell::RefCell;
use std::collections::HashMap;
use std::rc::Rc;

enum CachedBindings {
    Keys(Vec<Key>),
    Buttons(Vec<Button>),
}

impl CachedBindings {
    fn is_empty(&self) -> bool {
        match self {
            CachedBindings::Keys(list) => list.is_empty(),
            CachedBindings::Buttons(list) => list.is_empty(),
        }
    }
}

#[derive(Clone, Copy)]
pub enum BindingContextOption {
    Append,
    Clear,
}

pub struct UiConfigBinding {
    binds_keyboard: bool,
    input: Input,
    config: Rc<RefCell<Config>>,
    binding: bool,
    appending: bool,
    cached_bindings: CachedBindings,
    bound_text: String,
    text_scroller: OverflowTextScroller,
    context_receiver: Option<flume::Receiver<Option<BindingContextOption>>>,
    context_requester: Box<dyn Fn(flume::Sender<Option<BindingContextOption>>)>,
}

impl UiConfigBinding {
    fn new(input: Input, config: Rc<RefCell<Config>>, binds_keyboard: bool) -> Self {
        let mut ui = Self {
            binds_keyboard,
            input,
            config,
            binding: false,
            appending: false,
            cached_bindings: if binds_keyboard {
                CachedBindings::Keys(Vec::new())
            } else {
                CachedBindings::Buttons(Vec::new())
            },
            bound_text: String::new(),
            text_scroller: OverflowTextScroller::new(),
            context_receiver: None,
            context_requester: Box::new(|_| {}),
        };

        ui.regenerate_bound_text();

        ui
    }

    pub fn with_context_requester(
        mut self,
        callback: impl Fn(flume::Sender<Option<BindingContextOption>>) + 'static,
    ) -> Self {
        self.context_requester = Box::new(callback);
        self
    }

    pub fn new_keyboard(input: Input, config: Rc<RefCell<Config>>) -> Self {
        Self::new(input, config, true)
    }

    pub fn new_controller(input: Input, config: Rc<RefCell<Config>>) -> Self {
        Self::new(input, config, false)
    }

    fn regenerate_bound_text(&mut self) {
        let config = self.config.borrow();

        let binding_updated = match &self.cached_bindings {
            CachedBindings::Keys(keys) => config.key_bindings.get(&self.input) != Some(keys),
            CachedBindings::Buttons(buttons) => {
                config.controller_bindings.get(&self.input) != Some(buttons)
            }
        };

        if !binding_updated {
            return;
        }

        match &mut self.cached_bindings {
            CachedBindings::Keys(keys) => {
                let stored = config.key_bindings.get(&self.input);
                *keys = stored.cloned().unwrap_or_default();
            }
            CachedBindings::Buttons(buttons) => {
                let stored = config.controller_bindings.get(&self.input);
                *buttons = stored.cloned().unwrap_or_default();
            }
        };

        self.bound_text =
            Self::generate_bound_text(&config, self.binds_keyboard, self.input).unwrap_or_default();
    }

    fn generate_bound_text(config: &Config, binds_keyboard: bool, input: Input) -> Option<String> {
        if binds_keyboard {
            let keys = config.key_bindings.get(&input)?;

            if keys.is_empty() {
                return None;
            }

            Some(
                keys.iter()
                    .map(|key| -> &'static str { key.into() })
                    .join(","),
            )
        } else {
            let buttons = config.controller_bindings.get(&input)?;

            if buttons.is_empty() {
                return None;
            }

            let text = buttons
                .iter()
                .map(|button| match button {
                    Button::DPadUp => "D-Up",
                    Button::DPadDown => "D-Down",
                    Button::DPadLeft => "D-Left",
                    Button::DPadRight => "D-Right",
                    Button::LeftShoulder => "L-Shldr",
                    Button::LeftTrigger => "L-Trig",
                    Button::RightShoulder => "R-Shldr",
                    Button::RightTrigger => "R-Trig",
                    Button::LeftStick => "L-Stick",
                    Button::RightStick => "R-Stick",
                    Button::LeftStickUp => "L-Up",
                    Button::LeftStickDown => "L-Down",
                    Button::LeftStickLeft => "L-Left",
                    Button::LeftStickRight => "L-Right",
                    Button::RightStickUp => "R-Up",
                    Button::RightStickDown => "R-Down",
                    Button::RightStickLeft => "R-Left",
                    Button::RightStickRight => "R-Right",
                    button => button.into(),
                })
                .join(",");

            Some(text)
        }
    }

    fn bound_text_str(&self) -> &str {
        if self.binding {
            return "...";
        }

        &self.bound_text
    }
}

impl UiNode for UiConfigBinding {
    fn draw_bounded(
        &mut self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        bounds: Rect,
    ) {
        self.regenerate_bound_text();

        let mut text_style = TextStyle::new(game_io, FontStyle::Thick);
        text_style.shadow_color = TEXT_DARK_SHADOW_COLOR;
        text_style.bounds.set_position(bounds.position());

        if Input::REQUIRED.contains(&self.input) && self.cached_bindings.is_empty() {
            // display unbound required inputs in red
            text_style.color = Color::ORANGE;
        }

        // draw input name
        let text: &'static str = match self.input {
            Input::AdvanceFrame => "AdvFrame",
            Input::RewindFrame => "RewFrame",
            input => input.into(),
        };

        text_style.draw(game_io, sprite_queue, text);

        // draw binding
        let range = self.text_scroller.text_range(&self.bound_text);
        let text = &self.bound_text_str()[range];

        let metrics = text_style.measure(text);
        text_style.bounds.x += bounds.width - metrics.size.x - 1.0;

        text_style.draw(game_io, sprite_queue, text);
    }

    fn measure_ui_size(&mut self, _: &GameIO) -> Vec2 {
        Vec2::ZERO
    }

    fn focusable(&self) -> bool {
        true
    }

    fn is_locking_focus(&self) -> bool {
        self.binding || self.context_receiver.is_some()
    }

    fn update(&mut self, game_io: &mut GameIO, _bounds: Rect, focused: bool) {
        if !focused || self.binding {
            self.text_scroller.reset();
        }

        if !focused {
            return;
        }

        self.text_scroller.update();

        if let Some(receiver) = &self.context_receiver {
            if let Ok(selection) = receiver.try_recv() {
                // handle option
                match selection {
                    Some(BindingContextOption::Append) => {
                        self.binding = true;
                        self.appending = true;
                    }
                    Some(BindingContextOption::Clear) => {
                        let mut config = self.config.borrow_mut();

                        if self.binds_keyboard {
                            config.key_bindings.remove(&self.input);
                        } else {
                            config.controller_bindings.remove(&self.input);
                        }
                    }
                    None => {}
                }

                // clear receiver
                self.context_receiver = None;
            }

            // context menu is open, ignore input
            return;
        }

        if !self.binding {
            let input_util = InputUtil::new(game_io);

            // open context menu
            if input_util.was_just_pressed(Input::Option) {
                let (sender, receiver) = flume::unbounded();
                self.context_receiver = Some(receiver);
                (self.context_requester)(sender);
            }

            // begin new binding
            if input_util.was_just_pressed(Input::Confirm) {
                let globals = game_io.resource::<Globals>().unwrap();
                globals.audio.play_sound(&globals.cursor_select_sfx);

                self.binding = true;
                self.appending = false;
            }
            return;
        }

        if self.binds_keyboard {
            if let Some(key) = game_io.input().latest_key() {
                let mut config = self.config.borrow_mut();
                Self::bind(&mut config.key_bindings, self.input, key, self.appending);

                self.binding = false;
            }
        } else {
            if game_io.input().latest_key().is_some() {
                self.binding = false;
            }

            if let Some(button) = game_io.input().latest_button() {
                let mut config = self.config.borrow_mut();
                Self::bind(
                    &mut config.controller_bindings,
                    self.input,
                    button,
                    self.appending,
                );

                self.binding = false;
            }
        }
    }
}

impl UiConfigBinding {
    fn bind<V: std::cmp::PartialEq + Copy>(
        bindings: &mut HashMap<Input, Vec<V>>,
        input: Input,
        value: V,
        append: bool,
    ) {
        if Input::NON_OVERLAP.contains(&input) {
            // unbind overlapping input
            for (_, list) in bindings.iter_mut() {
                let Some(index) = list.iter().position(|v| *v == value) else {
                    continue;
                };

                list.remove(index);
            }
        }

        bindings
            .entry(input)
            .and_modify(|list| {
                if append {
                    if !list.contains(&value) {
                        list.push(value);
                    }
                } else {
                    *list = vec![value];
                }
            })
            .or_insert_with(|| vec![value]);
    }
}
