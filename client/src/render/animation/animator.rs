use super::{AnimationFrame, FrameList, FrameTime};
use crate::parse_util::parse_or_default;
use crate::resources::AssetManager;
use framework::graphics::Sprite;
use framework::prelude::{Rect, Vec2};
use std::collections::HashMap;

#[derive(Clone, Copy, PartialEq, Eq)]
pub enum AnimatorLoopMode {
    Once,
    Loop,
    Bounce,
}

#[derive(Clone)]
pub struct DerivedFrame {
    pub frame_index: usize,
    pub duration: FrameTime,
}

impl DerivedFrame {
    pub const fn new(frame_index: usize, duration: FrameTime) -> Self {
        Self {
            frame_index,
            duration,
        }
    }
}

#[derive(Clone)]
struct DerivedState {
    state: String,
    original_state: String,
    frame_derivation: Vec<DerivedFrame>,
}

#[derive(Clone)]
pub struct Animator {
    current_state: Option<String>,
    frame_index: usize,
    frame_progress: FrameTime,
    complete: bool,
    loop_mode: AnimatorLoopMode,
    loop_count: usize,
    bounced: bool,
    reversed: bool,
    states: HashMap<String, FrameList>,
    derived_states: Vec<DerivedState>,
}

impl Animator {
    pub fn new() -> Self {
        Self {
            current_state: None,
            frame_index: 0,
            frame_progress: 0,
            complete: true,
            loop_count: 0,
            loop_mode: AnimatorLoopMode::Once,
            bounced: false,
            reversed: false,
            states: HashMap::new(),
            derived_states: Vec::new(),
        }
    }

    pub fn load_new(assets: &impl AssetManager, path: &str) -> Self {
        let mut animator = Self::new();
        animator.load(assets, path);

        animator
    }

    pub fn with_state(mut self, state: &str) -> Self {
        self.set_state(state);
        self
    }

    pub fn with_loop_mode(mut self, loop_mode: AnimatorLoopMode) -> Self {
        self.set_loop_mode(loop_mode);
        self
    }

    pub fn copy_from(&mut self, other: &Self) {
        self.current_state = other.current_state.clone();
        self.frame_index = other.frame_index;
        self.frame_progress = other.frame_progress;
        self.complete = other.complete;
        self.loop_mode = other.loop_mode;
        self.bounced = other.bounced;
        self.reversed = other.reversed;
        self.states = other.states.clone();
        // skip derived states instructions as well
    }

    pub fn load(&mut self, assets: &impl AssetManager, path: &str) {
        let data = assets.text(path);

        self.load_from_str(&data);
    }

    pub fn load_from_str(&mut self, data: &str) {
        self.states.clear();

        let mut work_state: Option<(String, FrameList)> = None;
        let mut frame: Option<AnimationFrame> = None;

        for (i, line) in data.lines().enumerate() {
            let line = line.trim();

            if line.starts_with('#') {
                // comment
                continue;
            }

            // find the end of the word
            let word_boundary = |c| char::is_whitespace(c) || c == '=';
            let len = line.find(word_boundary).unwrap_or(line.len());
            let word = &line[..len];

            match word {
                "animation" => {
                    if let Some((state_name, mut frame_list)) = work_state.take() {
                        if let Some(frame) = frame.take() {
                            frame_list.add_frame(frame);
                        }

                        self.states.insert(state_name, frame_list);
                    }

                    let attributes = Animator::read_attributes(word, line, i);

                    let state_name = attributes
                        .get("state")
                        .cloned()
                        .unwrap_or_default()
                        .to_uppercase();

                    let frame_list = FrameList::default();

                    work_state = Some((state_name, frame_list));
                }
                "blank" => {
                    if let Some((_, frame_list)) = &mut work_state {
                        if let Some(frame) = frame.take() {
                            frame_list.add_frame(frame);
                        }

                        let attributes = Animator::read_attributes(word, line, i);

                        frame = Some(AnimationFrame {
                            duration: Animator::parse_duration_or_default(
                                attributes.get("duration").cloned(),
                            ),
                            valid: true,
                            ..AnimationFrame::default()
                        });
                    } else {
                        log::warn!("line {}: no animation state has been defined yet", i + 1);
                    }
                }
                "frame" => {
                    if let Some((_state, frame_list)) = &mut work_state {
                        if let Some(frame) = frame.take() {
                            frame_list.add_frame(frame);
                        }

                        let attributes = Animator::read_attributes(word, line, i);

                        let duration = Animator::parse_duration_or_default(
                            attributes.get("duration").cloned(),
                        );

                        let x: f32 = parse_or_default(attributes.get("x").cloned());
                        let y: f32 = parse_or_default(attributes.get("y").cloned());
                        let w: f32 = parse_or_default(attributes.get("w").cloned());
                        let h: f32 = parse_or_default(attributes.get("h").cloned());

                        let mut bounds = Rect::new(x, y, w, h);

                        let mut origin = Vec2::new(
                            parse_or_default(attributes.get("originx").cloned()),
                            parse_or_default(attributes.get("originy").cloned()),
                        );

                        let flip_x = parse_or_default::<u8>(attributes.get("flipx").cloned()) == 1;
                        let flip_y = parse_or_default::<u8>(attributes.get("flipy").cloned()) == 1;

                        if flip_x {
                            bounds.x = x + w;
                            bounds.width = -w;
                            origin.x = w - origin.x;
                        }

                        if flip_y {
                            bounds.y = y + h;
                            bounds.height = -h;
                            origin.y = h - origin.y;
                        }

                        frame = Some(AnimationFrame {
                            duration,
                            bounds,
                            origin,
                            points: HashMap::new(),
                            valid: true,
                        });
                    } else {
                        log::warn!("line {}: no animation state has been defined yet", i + 1);
                    }
                }
                "point" => {
                    if let Some(frame) = &mut frame {
                        let attributes = Animator::read_attributes(word, line, i);

                        let label = attributes
                            .get("label")
                            .map(|label| label.to_uppercase())
                            .unwrap_or_default();

                        let point = Vec2::new(
                            parse_or_default(attributes.get("x").cloned()),
                            parse_or_default(attributes.get("y").cloned()),
                        );

                        frame.points.insert(label, point);
                    } else {
                        log::warn!("line {}: no frame has been defined yet", i + 1);
                    }
                }
                "" | "VERSION" => {
                    // ignored
                }
                _ => {
                    if !word.starts_with("imagePath") {
                        log::warn!("line {}: unexpected {:?}", i + 1, word);
                    }
                }
            }
        }

        if let Some((state_name, mut frame_list)) = work_state.take() {
            if let Some(frame) = frame.take() {
                frame_list.add_frame(frame);
            }

            self.states.insert(state_name, frame_list);
        }

        self.rederive_states();

        if let Some(current_state) = self.current_state.take() {
            self.set_state(&current_state);
        }
    }

    fn parse_duration_or_default(duration_str: Option<&str>) -> FrameTime {
        let duration_str = match duration_str {
            Some(duration_str) => duration_str.trim(),
            _ => return 0,
        };

        if let Some(duration_str) = duration_str.strip_suffix('f') {
            return duration_str.parse().unwrap_or_default();
        }

        let seconds = duration_str.parse::<f32>().unwrap_or_default();
        let frames = (seconds * 60.0).round() as FrameTime;

        frames.max(0)
    }

    fn read_attributes<'a>(
        word: &str,
        line: &'a str,
        line_index: usize,
    ) -> HashMap<&'a str, &'a str> {
        let mut attributes = HashMap::new();
        let mut view = &line[word.len()..];

        loop {
            // skip whitespace
            view = view.trim_start();

            if view.is_empty() || view.starts_with('#') {
                break;
            }

            let eq_index = if let Some(index) = view.find('=') {
                index
            } else {
                log::warn!("line {}: expecting \"=\"", line_index + 1);
                break;
            };

            // read name
            let name = view[..eq_index].trim();

            // skip past =
            view = &view[eq_index + 1..];

            // skip whitespace
            view = view.trim_start();

            // read value
            let value = if view.starts_with('"') {
                if let Some(value) = Self::read_quoted(&mut view) {
                    value
                } else {
                    log::warn!("line {}: expecting matching '\"'", line_index + 1);
                    break;
                }
            } else {
                Self::read_unquoted(&mut view)
            };

            attributes.insert(name, value);
        }

        attributes
    }

    fn read_quoted<'a>(view: &mut &'a str) -> Option<&'a str> {
        // skip past first quote
        *view = &view[1..];

        let quote_index = view.find('"')?;

        // read value
        let value = &view[..quote_index];

        // skip past end quote
        *view = &view[quote_index + 1..];

        Some(value)
    }

    fn read_unquoted<'a>(view: &mut &'a str) -> &'a str {
        let end_index = match view.find(char::is_whitespace) {
            Some(index) => index,
            None => view.len(),
        };

        // read value
        let value = &view[..end_index];

        // skip past value
        // can't skip past whitespace as we could receive view.len() here
        *view = &view[end_index..];

        value
    }

    fn rederive_states(&mut self) {
        for data in &self.derived_states {
            let frame_list = self
                .derive_frames(&data.original_state, &data.frame_derivation)
                .unwrap_or_default();

            self.states.insert(data.state.clone(), frame_list);
        }
    }

    pub fn generate_state_id(state: &str) -> String {
        use std::time::{SystemTime, UNIX_EPOCH};

        let time_string = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap_or_default()
            .as_nanos()
            .to_string();

        state.to_owned() + "@" + &time_string
    }

    pub fn derive_state(
        &mut self,
        new_state: &str,
        original_state: &str,
        frame_derivation: Vec<DerivedFrame>,
    ) {
        let original_state = &original_state.to_uppercase();
        let frames = self
            .derive_frames(original_state, &frame_derivation)
            .unwrap_or_default();

        let derivation = DerivedState {
            state: new_state.to_uppercase(),
            original_state: original_state.to_owned(),
            frame_derivation,
        };

        self.states.insert(derivation.state.clone(), frames);
        self.derived_states.push(derivation);
    }

    /// Assumes state is uppercase
    fn derive_frames(&self, state: &str, frame_derivation: &[DerivedFrame]) -> Option<FrameList> {
        let original_frame_list = self.states.get(&state.to_uppercase())?;
        let mut frame_list = FrameList::default();

        for data in frame_derivation {
            let mut frame = original_frame_list
                .frame(data.frame_index)
                .cloned()
                .unwrap_or_default();

            frame.duration = data.duration;

            frame_list.add_frame(frame);
        }

        Some(frame_list)
    }

    pub fn add_state(&mut self, state: String, frame_list: FrameList) {
        self.states.insert(state, frame_list);
    }

    pub fn set_loop_mode(&mut self, loop_mode: AnimatorLoopMode) {
        if loop_mode != AnimatorLoopMode::Once {
            // looping, can't be completed anymore
            self.complete = false;
        }

        self.loop_mode = loop_mode;
    }

    pub fn loop_count(&self) -> usize {
        self.loop_count
    }

    pub fn set_reversed(&mut self, reversed: bool) {
        self.reversed = reversed;
    }

    pub fn current_state(&self) -> Option<&str> {
        self.current_state.as_deref()
    }

    pub fn has_state(&self, state: &str) -> bool {
        self.states.contains_key(&state.to_uppercase())
    }

    pub fn set_state(&mut self, state: &str) {
        // reset progress
        self.frame_index = 0;
        self.frame_progress = 0;
        self.bounced = false;
        self.complete = false;
        self.reversed = false;
        self.loop_mode = AnimatorLoopMode::Once;
        self.loop_count = 0;

        // update state
        let state = state.to_uppercase();

        if !self.states.contains_key(&state) {
            self.current_state = None;
        } else {
            self.current_state = Some(state);
        }
    }

    fn current_frame_list(&self) -> Option<&FrameList> {
        let state = self.current_state.as_ref()?.as_str();
        self.states.get(state)
    }

    fn current_frame_duration(&self) -> FrameTime {
        self.current_frame()
            .map(|frame| frame.duration)
            .unwrap_or_default()
    }

    pub fn current_frame(&self) -> Option<&AnimationFrame> {
        self.frame(self.frame_index)
    }

    pub fn current_frame_index(&self) -> usize {
        self.frame_index
    }

    pub fn frame(&self, index: usize) -> Option<&AnimationFrame> {
        self.current_frame_list()?.frame(index)
    }

    pub fn frame_list(&self, state: &str) -> Option<&FrameList> {
        self.states.get(&state.to_uppercase())
    }

    pub fn set_frame(&mut self, frame_index: usize) {
        self.frame_index = frame_index;

        if self.reversed {
            self.frame_progress = self
                .current_frame()
                .map(|frame| frame.duration)
                .unwrap_or_default();
        } else {
            self.frame_progress = 0;

            let total_frames = self
                .current_frame_list()
                .map(|frame_list| frame_list.frames().len())
                .unwrap_or_default();

            if self.frame_index >= total_frames {
                self.frame_index = total_frames.max(1) - 1;
            }
        }
    }

    pub fn origin(&self) -> Vec2 {
        self.current_frame()
            .map(|frame| frame.origin)
            .unwrap_or_default()
    }

    pub fn point(&self, name: &str) -> Option<Vec2> {
        self.current_frame()?.point(name)
    }

    pub fn apply(&self, sprite: &mut Sprite) {
        if let Some(frame) = self.current_frame() {
            frame.apply(sprite);
        }
    }

    pub fn is_complete(&self) -> bool {
        self.complete
    }

    pub fn sync_time(&mut self, mut time: FrameTime) {
        let frame_list = match self.current_frame_list() {
            Some(frame_list) => frame_list,
            None => return,
        };

        if frame_list.duration() == 0 {
            return;
        }

        if self.loop_mode == AnimatorLoopMode::Once && time > frame_list.duration() {
            if let Some(frame) = frame_list.frames().last() {
                self.frame_progress = frame.duration - 1;
            }

            self.complete = true;
            return;
        }

        let mut loops = time / frame_list.duration();
        time %= frame_list.duration();

        if time < 0 {
            // attempt to wrap around
            time += frame_list.duration();
            loops = loops.abs() + 1;
        }

        let forward = self.loop_mode != AnimatorLoopMode::Bounce || loops % 2 == 0;

        if forward {
            let mut frame_index = 0;

            for frame in frame_list.frames() {
                if time < frame.duration {
                    break;
                }

                time -= frame.duration;
                frame_index += 1;
            }

            self.frame_progress = time;
            self.frame_index = frame_index;
        } else {
            let mut frame_index = frame_list.frames().len() - 1;

            // flip the time
            time = frame_list.duration() - time;

            for frame in frame_list.frames().iter().rev() {
                if time < frame.duration {
                    break;
                }

                time -= frame.duration;
                frame_index -= 1;
            }

            // flip the time back
            self.frame_progress = frame_list.duration() - time;
            self.frame_index = frame_index;
        }

        self.bounced = !forward;
        self.complete = false;
    }

    fn calculate_increment(&self) -> FrameTime {
        let mut increment = 1;

        if self.bounced {
            increment *= -1;
        }

        if self.reversed {
            increment *= -1;
        }

        increment
    }

    pub fn update(&mut self) {
        if self.complete {
            return;
        }

        let frame_list = match self.current_frame_list() {
            Some(frame_list) => frame_list,
            // no active animation
            _ => return,
        };

        if frame_list.duration() == 0 {
            // would be an infinite loop if we allowed this to continue
            self.handle_completion();
            return;
        }

        self.frame_progress += self.calculate_increment();

        while !self.complete {
            let increment = self.calculate_increment();

            let frame_duration = self.current_frame_duration();

            if increment > 0 {
                if self.frame_progress < frame_duration {
                    // nothing to do
                    break;
                }

                self.frame_index += 1;

                let frame_list = self.current_frame_list().unwrap();

                if self.frame_index == frame_list.frames().len() {
                    // stay in bounds
                    self.frame_index -= 1;
                    self.frame_progress = frame_duration - 1;

                    self.handle_completion();
                } else {
                    self.frame_progress -= frame_duration;
                }
            } else {
                if self.frame_progress >= 0 {
                    // nothing to do
                    break;
                }

                if self.frame_index == 0 {
                    self.frame_progress = 0;
                    self.handle_completion();
                } else {
                    self.frame_index = self.frame_index.max(1) - 1;
                    self.frame_progress = self.current_frame_duration() - 1;
                }
            }
        }
    }

    fn handle_completion(&mut self) {
        if self.complete {
            // already completed
            return;
        }

        match self.loop_mode {
            AnimatorLoopMode::Once => {
                self.complete = true;
            }
            AnimatorLoopMode::Loop => {
                self.frame_index = 0;
                self.frame_progress = 0;
                self.loop_count += 1;
            }
            AnimatorLoopMode::Bounce => {
                if self.bounced {
                    // one full cycle
                    self.loop_count += 1;
                }

                self.bounced = !self.bounced;
            }
        }
    }
}

impl From<&str> for Animator {
    fn from(data: &str) -> Self {
        let mut animator = Self::new();
        animator.load_from_str(data);

        animator
    }
}

impl From<&String> for Animator {
    fn from(data: &String) -> Self {
        Animator::from(data.as_str())
    }
}

impl Default for Animator {
    fn default() -> Self {
        Self::new()
    }
}
