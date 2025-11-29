use super::{AnimationFrame, FrameList, FrameTime};
use crate::resources::AssetManager;
use framework::graphics::Sprite;
use framework::prelude::{Rect, Vec2};
use indexmap::IndexMap;
use structures::parse_util::parse_or_default;
use uncased::{Uncased, UncasedStr};

#[derive(Clone, Copy, PartialEq, Eq)]
pub enum AnimatorLoopMode {
    Once,
    Loop,
    Bounce,
}

#[derive(Clone)]
pub struct Animator {
    current_state: Option<Uncased<'static>>,
    frame_index: usize,
    frame_progress: FrameTime,
    complete: bool,
    loop_mode: AnimatorLoopMode,
    loop_count: usize,
    bounced: bool,
    reversed: bool,
    states: IndexMap<Uncased<'static>, FrameList>,
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
            states: IndexMap::new(),
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
        self.current_state.clone_from(&other.current_state);
        self.frame_index = other.frame_index;
        self.frame_progress = other.frame_progress;
        self.complete = other.complete;
        self.loop_mode = other.loop_mode;
        self.bounced = other.bounced;
        self.reversed = other.reversed;
        self.states.clone_from(&other.states);
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

            if line.starts_with(['#', '!']) {
                // comment or metadata
                continue;
            }

            // find the end of the word
            let word_boundary = |c| char::is_whitespace(c) || c == '=';
            let len = line.find(word_boundary).unwrap_or(line.len());
            let word = &line[..len];

            match word {
                "anim" | "animation" => {
                    if let Some((state_name, mut frame_list)) = work_state.take() {
                        if let Some(frame) = frame.take() {
                            frame_list.add_frame(frame);
                        }

                        self.states.insert(Uncased::from(state_name), frame_list);
                    }

                    let mut remaining_line = line[word.len()..].trim_start();

                    let state_name = if remaining_line.starts_with('"') {
                        let fallback = remaining_line;

                        Self::read_quoted(&mut remaining_line).unwrap_or(fallback)
                    } else if remaining_line
                        .strip_prefix("state")
                        .is_some_and(|t| t.trim_start().starts_with("="))
                    {
                        let attributes = Animator::read_attributes(remaining_line, i);

                        attributes.get("state").copied().unwrap_or_default()
                    } else {
                        remaining_line
                    };

                    let frame_list = FrameList::default();

                    work_state = Some((state_name.to_ascii_uppercase(), frame_list));
                }
                "blank" => {
                    if let Some((_, frame_list)) = &mut work_state {
                        if let Some(frame) = frame.take() {
                            frame_list.add_frame(frame);
                        }

                        let attributes = Animator::read_attributes(&line[word.len()..], i);

                        frame = Some(AnimationFrame {
                            duration: Animator::parse_duration_or_default(&attributes),
                            valid: true,
                            ..AnimationFrame::default()
                        });
                    } else {
                        log::warn!("Line {}: no animation state has been defined yet", i + 1);
                    }
                }
                "frame" => {
                    if let Some((_state, frame_list)) = &mut work_state {
                        if let Some(frame) = frame.take() {
                            frame_list.add_frame(frame);
                        }

                        let attributes = Animator::read_attributes(&line[word.len()..], i);

                        let duration = Animator::parse_duration_or_default(&attributes);

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
                            points: IndexMap::new(),
                            valid: true,
                        });
                    } else {
                        log::warn!("Line {}: no animation state has been defined yet", i + 1);
                    }
                }
                "point" => {
                    if let Some(frame) = &mut frame {
                        let mut remaining_line = line[word.len()..].trim_start();
                        let mut label = "";

                        if remaining_line.starts_with('"')
                            && let Some(value) = Self::read_quoted(&mut remaining_line)
                        {
                            label = value;
                        }

                        let attributes = Animator::read_attributes(remaining_line, i);

                        if let Some(value) = attributes.get("label") {
                            label = value;
                        }

                        let point = Vec2::new(
                            parse_or_default(attributes.get("x").cloned()),
                            parse_or_default(attributes.get("y").cloned()),
                        );

                        frame
                            .points
                            .insert(label.to_ascii_uppercase().into(), point);
                    } else {
                        log::warn!("Line {}: no frame has been defined yet", i + 1);
                    }
                }
                "" | "VERSION" => {
                    // ignored
                }
                _ => {
                    if !word.starts_with("imagePath") {
                        log::warn!("Line {}: unexpected {:?}", i + 1, word);
                    }
                }
            }
        }

        if let Some((state_name, mut frame_list)) = work_state.take() {
            if let Some(frame) = frame.take() {
                frame_list.add_frame(frame);
            }

            self.states.insert(Uncased::from(state_name), frame_list);
        }

        if let Some(current_state) = self.current_state.take() {
            // retain previous settings
            let loop_mode = self.loop_mode;
            let reversed = self.reversed;

            self.set_state(current_state.as_str());

            self.loop_mode = loop_mode;
            self.reversed = reversed;
        }
    }

    fn parse_duration_or_default(attributes: &IndexMap<&str, &str>) -> FrameTime {
        let duration_str = match attributes.get("duration").or(attributes.get("dur")) {
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

    fn read_attributes(mut view: &str, line_index: usize) -> IndexMap<&str, &str> {
        let mut attributes = IndexMap::new();

        loop {
            // skip whitespace
            view = view.trim_start();

            if view.is_empty() || view.starts_with('#') {
                break;
            }

            let Some(eq_index) = view.find('=') else {
                log::warn!("Line {}: expecting \"=\"", line_index + 1);
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
                    log::warn!("Line {}: expecting matching '\"'", line_index + 1);
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

    pub fn add_state<S: AsRef<str>>(&mut self, state: S, frame_list: FrameList) {
        let state = state.as_ref().to_ascii_uppercase().into();
        self.states.insert(state, frame_list);
    }

    pub fn remove_state(&mut self, state: &str) {
        let q: &UncasedStr = state.into();
        self.states.shift_remove(q);
    }

    pub fn loop_mode(&self) -> AnimatorLoopMode {
        self.loop_mode
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

    pub fn reversed(&self) -> bool {
        self.reversed
    }

    pub fn set_reversed(&mut self, reversed: bool) {
        self.reversed = reversed;
    }

    pub fn current_state(&self) -> Option<&str> {
        self.current_state.as_ref().map(|name| name.as_str())
    }

    pub fn has_state(&self, state: &str) -> bool {
        self.states.contains_key(&Uncased::from_borrowed(state))
    }

    pub fn iter_states(&self) -> impl Iterator<Item = (&str, &FrameList)> {
        self.states
            .iter()
            .map(|(state, frame_list)| (state.as_str(), frame_list))
    }

    pub fn set_state(&mut self, state: &str) -> bool {
        // reset progress
        self.frame_index = 0;
        self.frame_progress = 0;
        self.bounced = false;
        self.complete = false;
        self.reversed = false;
        self.loop_mode = AnimatorLoopMode::Once;
        self.loop_count = 0;

        // update state
        if let Some((key, _)) = self.states.get_key_value(<&UncasedStr>::from(state)) {
            self.current_state = Some(key.clone());
            true
        } else {
            self.current_state = None;
            false
        }
    }

    /// Calculates the current time for usage with sync_time
    pub fn calculate_time(&self) -> FrameTime {
        let Some(frame_list) = self.current_frame_list() else {
            return 0;
        };

        let reversed = self.reversed ^ self.bounced;

        let time = if reversed {
            let remaining_frames = &frame_list.frames()[self.frame_index..];
            let frame_iter = remaining_frames.iter();

            let frame_time = self.current_frame_duration() - self.frame_progress;
            frame_iter.fold(0, |acc, frame| acc + frame.duration) + frame_time
        } else {
            let remaining_frames = &frame_list.frames()[..self.frame_index];
            let frame_iter = remaining_frames.iter();

            frame_iter.fold(0, |acc, frame| acc + frame.duration) + self.frame_progress
        };

        if self.bounced {
            time + frame_list.duration()
        } else {
            time
        }
    }

    fn current_frame_list(&self) -> Option<&FrameList> {
        let state = self.current_state.as_ref()?;
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
        self.states.get(<&UncasedStr>::from(state))
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

    pub fn point_or_zero(&self, name: &str) -> Vec2 {
        self.point(name).unwrap_or_default()
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
        let Some(frame_list) = self.current_frame_list() else {
            return;
        };

        if frame_list.duration() == 0 {
            return;
        }

        if self.loop_mode == AnimatorLoopMode::Once && time >= frame_list.duration() {
            if self.reversed {
                self.frame_index = 0;
                self.frame_progress = 0;
            } else if let Some(frame) = frame_list.frames().last() {
                let last_index = frame_list.frames().len() - 1;

                self.frame_progress = frame.duration - 1;
                self.frame_index = last_index;
            }

            self.complete = true;
            return;
        }

        let mut loops = time / frame_list.duration();
        time = time.checked_rem(frame_list.duration()).unwrap_or_default();

        if time < 0 {
            // attempt to wrap around
            time += frame_list.duration();
            loops = loops.abs() + 1;
        }

        let forward = self.loop_mode != AnimatorLoopMode::Bounce || loops % 2 == 0;

        if forward ^ self.reversed {
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
            let frame_len = frame_list.frames().len();
            let mut frame_index = frame_len;

            for frame in frame_list.frames().iter().rev() {
                if time < frame.duration {
                    break;
                }

                time -= frame.duration;
                frame_index -= 1;
            }

            // flip the time back
            self.frame_progress = frame_list.duration() - time;
            self.frame_index = frame_index.min(frame_len.saturating_sub(1));
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

        let Some(frame_list) = self.current_frame_list() else {
            // no active animation
            return;
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

#[cfg(test)]
mod test {
    use super::*;

    const ANIMATION_STR: &str = r#"
        animation state="a"

        animation state="B"
        frame originx="64"
        point label="P1"

        animation "C"
        blank
        point "P2"

        anim D
    "#;

    #[test]
    fn capitalized_states() {
        let mut animator = Animator::new();
        animator.load_from_str(ANIMATION_STR);

        let state_names = animator
            .iter_states()
            .map(|(name, _)| name)
            .collect::<Vec<_>>();

        assert_eq!(state_names, ["A", "B", "C", "D"]);
    }

    #[test]
    fn insensitive_has_and_set_state() {
        let mut animator = Animator::new();
        animator.load_from_str(ANIMATION_STR);

        assert!(animator.has_state("a"));

        animator.set_state("b");
        assert_eq!(animator.origin().x, 64.0);
    }

    #[test]
    fn point_labels() {
        let mut animator = Animator::new();
        animator.load_from_str(ANIMATION_STR);

        let point_labels = animator
            .iter_states()
            .flat_map(|(_, state)| state.frames())
            .flat_map(|frame| frame.points.keys())
            .map(|s| s.as_str())
            .collect::<Vec<_>>();

        assert_eq!(point_labels, ["P1", "P2"]);
    }
}
