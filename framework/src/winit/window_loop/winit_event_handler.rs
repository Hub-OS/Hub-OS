use winit::event::Event as WinitEvent;
use winit::event_loop::ControlFlow;

pub(super) trait WinitEventHandler {
  fn handle_event(
    &mut self,
    winit_event: WinitEvent<'_, ()>,
    control_flow: &mut ControlFlow,
  ) -> Option<Box<dyn WinitEventHandler>>;
}
