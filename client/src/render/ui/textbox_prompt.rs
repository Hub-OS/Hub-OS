use super::*;
use crate::render::*;
use crate::resources::TEXTBOX_TEXT_COLOR;
use framework::prelude::*;

pub struct TextboxPrompt {
    layout: Option<UiLayout>,
    character_limit: Option<usize>,
    initial_text: String,
    ui_input_tracker: UiInputTracker,
    text_sender: flume::Sender<String>,
    text_receiver: flume::Receiver<String>,
    filter: Option<Box<dyn Fn(&str) -> bool>>,
    callback: Option<Box<dyn FnOnce(String)>>,
}

impl TextboxPrompt {
    pub fn new(callback: impl FnOnce(String) + 'static) -> Self {
        let (text_sender, text_receiver) = flume::unbounded();

        Self {
            layout: None,
            character_limit: None,
            initial_text: String::new(),
            ui_input_tracker: UiInputTracker::new(),
            text_sender,
            text_receiver,
            filter: None,
            callback: Some(Box::new(callback)),
        }
    }

    pub fn with_str(mut self, text: &str) -> Self {
        self.initial_text = text.to_string();
        self
    }

    pub fn with_character_limit(mut self, limit: usize) -> Self {
        self.character_limit = Some(limit);
        self
    }

    pub fn with_filter(mut self, callback: impl Fn(&str) -> bool + 'static) -> Self {
        self.filter = Some(Box::new(callback));
        self
    }
}

impl TextboxInterface for TextboxPrompt {
    fn text(&self) -> &str {
        ""
    }

    fn hides_avatar(&self) -> bool {
        true
    }

    fn is_complete(&self) -> bool {
        self.callback.is_none()
    }

    fn update(&mut self, game_io: &mut GameIO, text_style: &TextStyle, _lines: usize) {
        if let Some(layout) = &mut self.layout {
            layout.set_position(text_style.bounds.position());
            layout.update(game_io, &self.ui_input_tracker);

            if !layout.is_focus_locked() {}
        } else {
            let sender = self.text_sender.clone();
            let mut text_input = TextInput::new(game_io, text_style.font.clone())
                .with_str(&self.initial_text)
                .with_paged(text_style.bounds.size())
                .with_color(TEXTBOX_TEXT_COLOR)
                .with_silent(true)
                .with_active(true)
                .on_change(move |value| sender.send(value.to_string()).unwrap());

            if let Some(filter) = self.filter.take() {
                text_input = text_input.with_filter(filter);
            }

            if let Some(character_limit) = self.character_limit {
                text_input = text_input.with_character_limit(character_limit);
            }

            let layout =
                UiLayout::new_vertical(text_style.bounds, vec![UiLayoutNode::new(text_input)]);

            self.layout = Some(layout);
        }

        if let Ok(value) = self.text_receiver.try_recv() {
            let callback = self.callback.take().unwrap();
            callback(value);
        }
    }

    fn draw(&mut self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        if let Some(layout) = &mut self.layout {
            layout.draw(game_io, sprite_queue);
        }
    }
}
