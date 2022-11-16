use crate::prelude::*;
use winit::event::Event as WinitEvent;
use winit::event::StartCause as WinitEventStartCause;
use winit::event_loop::ControlFlow;
use winit::window::WindowId;

pub(super) struct ActiveHandler<Globals: 'static> {
    window_id: WindowId,
    game_runtime: GameRuntime<Globals>,
    controller_event_pump: ControllerEventPump,
}

impl<Globals> ActiveHandler<Globals> {
    pub(super) async fn new(
        window: Window,
        loop_params: WindowLoopParams<Globals>,
    ) -> anyhow::Result<Self> {
        let window_id = window.id();
        let mut game_runtime = GameRuntime::new(window, loop_params).await?;
        let controller_event_pump = ControllerEventPump::new(&mut game_runtime)?;

        Ok(Self {
            window_id,
            game_runtime,
            controller_event_pump,
        })
    }
}

impl<Globals> super::WinitEventHandler for ActiveHandler<Globals> {
    fn handle_event(
        &mut self,
        winit_event: WinitEvent<'_, ()>,
        control_flow: &mut ControlFlow,
    ) -> Option<Box<dyn super::WinitEventHandler>> {
        if self.game_runtime.is_quitting() {
            control_flow.set_exit();
            return None;
        }

        match winit_event {
            WinitEvent::NewEvents(WinitEventStartCause::Init)
            | WinitEvent::NewEvents(WinitEventStartCause::ResumeTimeReached { .. }) => {
                self.controller_event_pump.pump(&mut self.game_runtime);
                self.game_runtime.tick();
                control_flow.set_wait_until(self.game_runtime.target_sleep_instant());
            }
            _ => {
                if let Some(event) = translate_winit_event(self.window_id, winit_event) {
                    self.game_runtime.push_event(event)
                }
            }
        }

        None
    }
}
