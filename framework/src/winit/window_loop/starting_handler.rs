use super::*;
use crate::cfg_android;
use crate::logging::*;
use crate::prelude::*;
use winit::event::Event as WinitEvent;
use winit::event_loop::ControlFlow;

struct StartingHandlerInitParams<Globals: 'static> {
    window: Window,
    loop_params: WindowLoopParams<Globals>,
}

pub(super) struct StartingHandler<Globals: 'static> {
    async_executor: async_executor::LocalExecutor<'static>,
    starting_handler_init_params: Option<StartingHandlerInitParams<Globals>>,
    task: Option<async_executor::Task<anyhow::Result<ActiveHandler<Globals>>>>,
}

impl<Globals> StartingHandler<Globals> {
    pub(super) fn new(window: Window, loop_params: WindowLoopParams<Globals>) -> Self {
        #[allow(unused_mut)]
        let mut starting_handler = Self {
            async_executor: async_executor::LocalExecutor::new(),
            starting_handler_init_params: Some(StartingHandlerInitParams {
                window,
                loop_params,
            }),
            task: None,
        };

        crate::cfg_desktop_and_web!({
            starting_handler.start_active_handler_task();
        });

        starting_handler
    }
}

impl<Globals> StartingHandler<Globals> {
    fn start_active_handler_task(&mut self) {
        if let Some(params) = self.starting_handler_init_params.take() {
            self.task = Some(
                self.async_executor
                    .spawn(ActiveHandler::new(params.window, params.loop_params)),
            );
        }
    }
}

impl<Globals> WinitEventHandler for StartingHandler<Globals> {
    fn handle_event(
        &mut self,
        winit_event: WinitEvent<'_, ()>,
        control_flow: &mut ControlFlow,
    ) -> Option<Box<dyn WinitEventHandler>> {
        match winit_event {
            WinitEvent::Resumed => {
                cfg_android!(self.start_active_handler_task())
            }
            WinitEvent::WindowEvent {
                event: winit::event::WindowEvent::CloseRequested,
                ..
            } => {
                *control_flow = ControlFlow::Exit;
            }
            WinitEvent::MainEventsCleared => {
                while self.async_executor.try_tick() {}

                let task_ref = self.task.as_ref();
                let task_completed = task_ref.map(|task| task.is_finished()).unwrap_or_default();

                if task_completed {
                    let task = self.task.take().unwrap();
                    let task_value = pollster::block_on(task.cancel()).unwrap();

                    match task_value {
                        Ok(new_handler) => {
                            return Some(Box::new(new_handler));
                        }
                        Err(e) => {
                            error!("{}", e);
                            *control_flow = ControlFlow::Exit;
                        }
                    }
                }
            }
            _ => {}
        }

        None
    }
}
