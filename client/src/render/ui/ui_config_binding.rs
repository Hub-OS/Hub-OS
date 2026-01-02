use super::{FontName, OverflowTextScroller, TextStyle, UiNode};
use crate::render::{FrameTime, SpriteColorQueue};
use crate::resources::*;
use crate::saves::Config;
use framework::prelude::*;
use itertools::Itertools;
use std::cell::RefCell;
use std::collections::HashMap;
use std::rc::Rc;

const MAX_WAIT_TIME: FrameTime = 60 * 5;

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

struct BindingState {
    appending: bool,
    open_time: FrameTime,
}

pub struct UiConfigBinding {
    input: Input,
    config: Rc<RefCell<Config>>,
    binding_state: Option<Box<BindingState>>,
    cached_bindings: CachedBindings,
    label: String,
    bound_text: String,
    text_scroller: OverflowTextScroller,
    context_receiver: Option<flume::Receiver<Option<BindingContextOption>>>,
    context_requester: Box<dyn Fn(flume::Sender<Option<BindingContextOption>>)>,
}

impl UiConfigBinding {
    fn new(
        game_io: &GameIO,
        input: Input,
        config: Rc<RefCell<Config>>,
        binds_keyboard: bool,
    ) -> Self {
        let globals = Globals::from_resources(game_io);

        let mut ui = Self {
            input,
            config,
            binding_state: None,
            cached_bindings: if binds_keyboard {
                CachedBindings::Keys(Vec::new())
            } else {
                CachedBindings::Buttons(Vec::new())
            },
            label: globals.translate(input.translation_key()),
            bound_text: String::new(),
            text_scroller: OverflowTextScroller::new(),
            context_receiver: None,
            context_requester: Box::new(|_| {}),
        };

        ui.regenerate_bound_text(game_io);

        ui
    }

    pub fn with_context_requester(
        mut self,
        callback: impl Fn(flume::Sender<Option<BindingContextOption>>) + 'static,
    ) -> Self {
        self.context_requester = Box::new(callback);
        self
    }

    pub fn new_keyboard(game_io: &GameIO, input: Input, config: Rc<RefCell<Config>>) -> Self {
        Self::new(game_io, input, config, true)
    }

    pub fn new_controller(game_io: &GameIO, input: Input, config: Rc<RefCell<Config>>) -> Self {
        Self::new(game_io, input, config, false)
    }

    fn regenerate_bound_text(&mut self, game_io: &GameIO) {
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
            Self::generate_bound_text(game_io, &config, self.binds_keyboard(), self.input)
                .unwrap_or_default();
    }

    fn generate_bound_text(
        game_io: &GameIO,
        config: &Config,
        binds_keyboard: bool,
        input: Input,
    ) -> Option<String> {
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

            let globals = Globals::from_resources(game_io);

            let text = buttons
                .iter()
                .map(|button| globals.translate(Translations::button_short_key(*button)))
                .join(",");

            Some(text)
        }
    }

    fn displayed_text(&self) -> &str {
        let text = if self.binding_state.is_some() {
            "..."
        } else {
            &self.bound_text
        };

        let range = self.text_scroller.text_range(text);

        &text[range]
    }

    fn binds_keyboard(&self) -> bool {
        matches!(self.cached_bindings, CachedBindings::Keys(_))
    }
}

impl UiNode for UiConfigBinding {
    fn draw_bounded(
        &mut self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        bounds: Rect,
    ) {
        self.regenerate_bound_text(game_io);

        let mut text_style = TextStyle::new(game_io, FontName::Thick);
        text_style.shadow_color = TEXT_DARK_SHADOW_COLOR;
        text_style.bounds.set_position(bounds.position());

        if Input::REQUIRED.contains(&self.input) && self.cached_bindings.is_empty() {
            // display unbound required inputs in red
            text_style.color = Color::ORANGE;
        }

        // draw input name
        text_style.draw(game_io, sprite_queue, &self.label);

        // draw binding
        let text = self.displayed_text();

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
        self.binding_state.is_some() || self.context_receiver.is_some()
    }

    fn update(&mut self, game_io: &mut GameIO, _bounds: Rect, focused: bool) {
        if !focused || self.binding_state.is_some() {
            self.text_scroller.reset();
        }

        if !focused {
            return;
        }

        self.text_scroller.update();

        let binds_keyboard = self.binds_keyboard();

        if let Some(receiver) = &self.context_receiver {
            if let Ok(selection) = receiver.try_recv() {
                // handle option
                match selection {
                    Some(BindingContextOption::Append) => {
                        self.binding_state = Some(Box::new(BindingState {
                            appending: true,
                            open_time: 0,
                        }));
                    }
                    Some(BindingContextOption::Clear) => {
                        let mut config = self.config.borrow_mut();

                        if binds_keyboard {
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

        let Some(state) = &mut self.binding_state else {
            let input_util = InputUtil::new(game_io);

            // open context menu
            if input_util.was_just_pressed(Input::Option) {
                let (sender, receiver) = flume::unbounded();
                self.context_receiver = Some(receiver);
                (self.context_requester)(sender);
            }

            // begin new binding
            if input_util.was_just_pressed(Input::Confirm) {
                let globals = Globals::from_resources(game_io);
                globals.audio.play_sound(&globals.sfx.cursor_select);

                self.binding_state = Some(Box::new(BindingState {
                    appending: false,
                    open_time: 0,
                }));
            }

            return;
        };

        // time out to avoid softlocking when a keyboard or controller isn't present
        state.open_time += 1;

        if state.open_time > MAX_WAIT_TIME {
            self.binding_state = None;
            return;
        }

        // check input
        if binds_keyboard {
            if let Some(key) = game_io.input().latest_key() {
                let mut config = self.config.borrow_mut();
                Self::bind(&mut config.key_bindings, self.input, key, state.appending);

                self.binding_state = None;
            }
        } else {
            let globals = Globals::from_resources(game_io);
            let latest_button = game_io.input().latest_button();
            let latest_button = latest_button.or(globals.emulated_input.latest_button());

            if let Some(button) = latest_button {
                let mut config = self.config.borrow_mut();
                Self::bind(
                    &mut config.controller_bindings,
                    self.input,
                    button,
                    state.appending,
                );

                self.binding_state = None;
            } else if game_io.input().latest_key().is_some() {
                // cancel controller binding with any keyboard press
                self.binding_state = None;
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
