use crate::prelude::*;
use gilrs::ev::EventType as GilRsEvent;
use std::cell::RefCell;
use std::rc::Rc;
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

struct ControllerEventPump {
    gilrs: Rc<RefCell<gilrs::Gilrs>>,
}

impl ControllerEventPump {
    fn new<Globals>(game_runtime: &mut GameRuntime<Globals>) -> anyhow::Result<Self> {
        let gilrs = gilrs::Gilrs::new().map_err(|e| {
            crate::logging::error!("{e}");
            anyhow::anyhow!("Failed to initialize game controller subsystem")
        })?;

        let gilrs = Rc::new(RefCell::new(gilrs));

        for (id, _) in gilrs.borrow().gamepads() {
            let input_event = InputEvent::ControllerConnected {
                controller_id: id.into(),
                rumble_pack: RumblePack::new(gilrs.clone(), id),
            };

            game_runtime.push_event(input_event.into());
        }

        Ok(Self { gilrs })
    }

    fn pump<Globals>(&mut self, game_runtime: &mut GameRuntime<Globals>) {
        // Examine new events
        while let Some(gilrs::Event { id, event, time: _ }) = self.gilrs.borrow_mut().next_event() {
            let input_event = convert_event(&self.gilrs, id, event);

            if let Some(input_event) = input_event {
                game_runtime.push_event(input_event.into());
            }
        }
    }
}

fn convert_event(
    gilrs: &Rc<RefCell<gilrs::Gilrs>>,
    id: gilrs::GamepadId,
    event: GilRsEvent,
) -> Option<InputEvent> {
    match event {
        GilRsEvent::Connected => Some(InputEvent::ControllerConnected {
            controller_id: id.into(),
            rumble_pack: RumblePack::new(gilrs.clone(), id),
        }),
        GilRsEvent::Disconnected => Some(InputEvent::ControllerDisconnected(id.into())),
        GilRsEvent::ButtonPressed(button, _) => Some(InputEvent::ControllerButtonDown {
            controller_id: id.into(),
            button: convert_button(button)?,
        }),
        GilRsEvent::ButtonReleased(button, _) => Some(InputEvent::ControllerButtonUp {
            controller_id: id.into(),
            button: convert_button(button)?,
        }),
        GilRsEvent::AxisChanged(axis, value, _) => Some(InputEvent::ControllerAxis {
            controller_id: id.into(),
            axis: convert_axis(axis)?,
            value,
        }),
        _ => None,
    }
}

fn convert_button(button: gilrs::Button) -> Option<Button> {
    match button {
        gilrs::Button::Mode => Some(Button::Meta),
        gilrs::Button::Start => Some(Button::Start),
        gilrs::Button::Select => Some(Button::Select),
        gilrs::Button::South => Some(Button::A),
        gilrs::Button::West => Some(Button::B),
        gilrs::Button::East => Some(Button::X),
        gilrs::Button::North => Some(Button::Y),
        gilrs::Button::C => Some(Button::C),
        gilrs::Button::Z => Some(Button::Z),
        gilrs::Button::LeftTrigger => Some(Button::LeftShoulder),
        gilrs::Button::LeftTrigger2 => Some(Button::LeftTrigger),
        gilrs::Button::RightTrigger => Some(Button::RightShoulder),
        gilrs::Button::RightTrigger2 => Some(Button::RightTrigger),
        gilrs::Button::LeftThumb => Some(Button::LeftStick),
        gilrs::Button::RightThumb => Some(Button::RightStick),
        gilrs::Button::DPadUp => Some(Button::DPadUp),
        gilrs::Button::DPadDown => Some(Button::DPadDown),
        gilrs::Button::DPadLeft => Some(Button::DPadLeft),
        gilrs::Button::DPadRight => Some(Button::DPadRight),
        gilrs::Button::Unknown => None,
    }
}

fn convert_axis(axis: gilrs::Axis) -> Option<AnalogAxis> {
    match axis {
        gilrs::Axis::LeftStickX => Some(AnalogAxis::LeftStickX),
        gilrs::Axis::LeftStickY => Some(AnalogAxis::LeftStickY),
        gilrs::Axis::RightStickX => Some(AnalogAxis::RightStickX),
        gilrs::Axis::RightStickY => Some(AnalogAxis::RightStickY),
        gilrs::Axis::LeftZ => Some(AnalogAxis::LeftTrigger),
        gilrs::Axis::RightZ => Some(AnalogAxis::RightTrigger),
        _ => None,
    }
}
