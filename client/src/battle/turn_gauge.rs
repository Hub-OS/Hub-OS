use crate::render::FrameTime;

#[derive(Clone)]
pub struct TurnGauge {
    time: FrameTime,
    max_time: FrameTime,
}

impl TurnGauge {
    pub const DEFAULT_MAX_TIME: FrameTime = 512;

    pub fn new() -> Self {
        Self {
            time: 0,
            max_time: Self::DEFAULT_MAX_TIME,
        }
    }

    pub fn increment_time(&mut self) {
        self.time = self.max_time.min(self.time + 1);
    }

    pub fn progress(&self) -> f32 {
        if self.max_time == 0 {
            1.0
        } else {
            self.time as f32 / self.max_time as f32
        }
    }

    pub fn time(&self) -> FrameTime {
        self.time
    }

    pub fn set_time(&mut self, time: FrameTime) {
        self.time = time.min(self.max_time)
    }

    pub fn max_time(&self) -> FrameTime {
        self.max_time
    }

    pub fn set_max_time(&mut self, time: FrameTime) {
        self.max_time = time
    }
}

impl Default for TurnGauge {
    fn default() -> Self {
        Self::new()
    }
}
