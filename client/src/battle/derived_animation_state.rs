use crate::render::{AnimationFrame, Animator, FrameList, FrameTime};

#[derive(Clone)]
pub struct DerivedAnimationFrame {
    pub frame_index: usize,
    pub duration: FrameTime,
}

impl DerivedAnimationFrame {
    pub const fn new(frame_index: usize, duration: FrameTime) -> Self {
        Self {
            frame_index,
            duration,
        }
    }
}

#[derive(Clone)]
pub struct DerivedAnimationState {
    pub state: String,
    pub original_state: String,
    pub frame_derivation: Vec<DerivedAnimationFrame>,
}

impl DerivedAnimationState {
    pub fn new(original_state: &str, frame_derivation: Vec<DerivedAnimationFrame>) -> Self {
        Self {
            state: Self::generate_state_id(original_state),
            original_state: original_state.to_string(),
            frame_derivation,
        }
    }

    fn generate_state_id(state: &str) -> String {
        use std::time::{SystemTime, UNIX_EPOCH};

        let time_string = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap_or_default()
            .as_nanos()
            .to_string();

        state.to_owned() + "@" + &time_string
    }

    pub fn apply(&self, animator: &mut Animator) {
        let mut frame_list = FrameList::default();

        if let Some(original_frame_list) = animator.frame_list(&self.original_state) {
            for data in &self.frame_derivation {
                let mut frame = original_frame_list
                    .frame(data.frame_index)
                    .cloned()
                    .unwrap_or_default();

                frame.duration = data.duration;

                frame_list.add_frame(frame);
            }
        } else {
            for data in &self.frame_derivation {
                let frame = AnimationFrame {
                    duration: data.duration,
                    ..Default::default()
                };

                frame_list.add_frame(frame);
            }
        }

        animator.add_state(&self.state, frame_list);
    }
}
