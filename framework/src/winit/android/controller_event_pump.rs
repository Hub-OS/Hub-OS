use super::RumblePack;
use crate::prelude::{AnalogAxis, Button, GameRuntime, InputEvent};

pub(crate) struct ControllerEventPump {}

impl ControllerEventPump {
    pub(crate) fn new<Globals>(game_runtime: &mut GameRuntime<Globals>) -> anyhow::Result<Self> {
        Ok(Self {})
    }

    pub(crate) fn pump<Globals>(&mut self, game_runtime: &mut GameRuntime<Globals>) {}
}
