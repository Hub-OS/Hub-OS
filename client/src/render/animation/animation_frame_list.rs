use super::{AnimationFrame, FrameTime};

#[derive(Default, Clone)]
pub struct FrameList {
    frames: Vec<AnimationFrame>,
    duration: FrameTime,
}

impl FrameList {
    pub fn frames(&self) -> &[AnimationFrame] {
        &self.frames
    }

    pub fn frame(&self, index: usize) -> Option<&AnimationFrame> {
        self.frames.get(index)
    }

    pub fn duration(&self) -> FrameTime {
        self.duration
    }

    pub fn add_frame(&mut self, mut frame: AnimationFrame) {
        frame.duration = frame.duration.max(0);

        self.duration += frame.duration;
        self.frames.push(frame);
    }
}

impl From<Vec<AnimationFrame>> for FrameList {
    fn from(mut frames: Vec<AnimationFrame>) -> Self {
        let mut duration = 0;

        for frame in &mut frames {
            frame.duration = frame.duration.max(0);
            duration += frame.duration;
        }

        Self { frames, duration }
    }
}
