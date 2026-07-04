use crate::net::Net;
use crate::plugins::PluginInterface;
use reedline::{ExternalPrinter, MenuBuilder, ReedlineEvent, ReedlineMenu};
use std::borrow::Cow;
use std::sync::atomic::AtomicBool;
use std::sync::{Arc, Mutex};

const ATOMIC_ORDERING: std::sync::atomic::Ordering = std::sync::atomic::Ordering::Relaxed;

// custom prompt to look more like a repl and less like a shell
struct CustomPrompt;
impl reedline::Prompt for CustomPrompt {
    fn render_prompt_left(&self) -> Cow<'_, str> {
        Cow::Borrowed("")
    }

    fn render_prompt_right(&self) -> Cow<'_, str> {
        Cow::Borrowed("")
    }

    fn render_prompt_indicator(&self, _edit_mode: reedline::PromptEditMode) -> Cow<'_, str> {
        Cow::Borrowed("> ")
    }

    fn render_prompt_multiline_indicator(&self) -> Cow<'_, str> {
        Cow::Borrowed(">> ")
    }

    fn render_prompt_history_search_indicator(
        &self,
        history_search: reedline::PromptHistorySearch,
    ) -> Cow<'_, str> {
        use reedline::PromptHistorySearchStatus;

        let prefix = match history_search.status {
            PromptHistorySearchStatus::Passing => "",
            PromptHistorySearchStatus::Failing => "failing ",
        };

        Cow::Owned(format!(
            "({}reverse-search: {}) ",
            prefix, history_search.term
        ))
    }
}

// wrapper over the default completer to pull in
struct CustomCompleter {
    commands: Vec<String>,
    new_commands: Arc<Mutex<Vec<String>>>,
}

impl CustomCompleter {
    fn new() -> (Self, Arc<Mutex<Vec<String>>>) {
        let new_commands = Arc::new(Mutex::new(Default::default()));

        (
            CustomCompleter {
                commands: Default::default(),
                new_commands: new_commands.clone(),
            },
            new_commands,
        )
    }

    fn receive_completions(&mut self) {
        let Ok(mut guard) = self.new_commands.lock() else {
            return;
        };

        if !guard.is_empty() {
            std::mem::swap(&mut *guard, &mut self.commands);
        }
    }
}

impl reedline::Completer for CustomCompleter {
    fn complete(&mut self, line: &str, pos: usize) -> Vec<reedline::Suggestion> {
        self.receive_completions();

        if pos != line.len() {
            return vec![];
        }

        let reference_str = line.trim_start();

        self.commands
            .iter()
            .filter(|command_name| command_name.starts_with(reference_str))
            .map(|command_name| reedline::Suggestion {
                value: command_name.clone(),
                span: reedline::Span {
                    start: 0,
                    end: line.len(),
                },
                ..Default::default()
            })
            .collect()
    }
}

enum Event {
    Command(String),
    Stop,
}

pub struct ReadLinePlugin {
    prev_command_count: usize,
    new_commands: Arc<Mutex<Vec<String>>>,
    event_receiver: flume::Receiver<Event>,
    break_signal: Arc<AtomicBool>,
    join_handle: Option<std::thread::JoinHandle<()>>,
    external_printer: ExternalPrinter<String>,
}

impl ReadLinePlugin {
    pub fn new() -> Self {
        // take over printing to avoid threading issues
        let external_printer = reedline::ExternalPrinter::new(1000);
        let printer_sender = external_printer.sender();

        crate::logger::set_printer(move |message| {
            let _ = printer_sender.send(message);
        });

        // build reedline's readline
        let (command_sender, event_receiver) = flume::unbounded();
        let (custom_completer, new_commands) = CustomCompleter::new();
        let break_signal = Arc::new(AtomicBool::new(false));
        let printer = external_printer.clone();

        let mut keybindings = reedline::default_emacs_keybindings();
        keybindings.add_binding(
            reedline::KeyModifiers::NONE,
            reedline::KeyCode::Tab,
            ReedlineEvent::UntilFound(vec![
                ReedlineEvent::Menu("completion_menu".to_string()),
                ReedlineEvent::MenuNext,
            ]),
        );

        let completion_menu = Box::new(reedline::IdeMenu::default().with_name("completion_menu"));

        let mut line_editor = reedline::Reedline::create()
            .with_external_printer(external_printer)
            .with_break_signal(break_signal.clone())
            .with_edit_mode(Box::new(reedline::Emacs::new(keybindings)))
            .with_menu(ReedlineMenu::EngineCompleter(completion_menu))
            .with_completer(Box::new(custom_completer))
            .with_partial_completions(true)
            .with_quick_completions(true);

        let join_handle = std::thread::spawn(move || {
            loop {
                let sig = line_editor.read_line(&CustomPrompt);

                match sig {
                    Ok(reedline::Signal::Success(command)) => {
                        let _ = command_sender.send(Event::Command(command));
                    }
                    Ok(reedline::Signal::CtrlD) | Ok(reedline::Signal::CtrlC) => {
                        let _ = command_sender.send(Event::Stop);
                        break;
                    }
                    Ok(reedline::Signal::ExternalBreak(_)) => {
                        break;
                    }
                    Err(err) => {
                        log::error!("{err:?}");
                        let _ = command_sender.send(Event::Stop);
                        break;
                    }
                    _ => {}
                }
            }
        });

        Self {
            prev_command_count: 0,
            new_commands,
            event_receiver,
            break_signal,
            join_handle: Some(join_handle),
            external_printer: printer,
        }
    }
}

impl Drop for ReadLinePlugin {
    fn drop(&mut self) {
        // stop the readline thread
        self.break_signal.store(true, ATOMIC_ORDERING);

        if let Some(handle) = self.join_handle.take() {
            let _ = handle.join();
        }

        // switch printer, now that reedline isn't handling our logs
        crate::logger::set_printer(|s| println!("{s}"));

        // flush remaining messages
        while let Ok(message) = self.external_printer.receiver().try_recv() {
            println!("{message}")
        }
    }
}

impl PluginInterface for ReadLinePlugin {
    fn tick(&mut self, net: &mut Net, _: f32) {
        // see if any new commands were created since the last tick
        let command_registry = net.command_registry_mut();

        let command_count = command_registry.len();

        if command_count != self.prev_command_count {
            // update command list
            self.prev_command_count = command_registry.len();

            if let Ok(mut commands) = self.new_commands.lock() {
                commands.extend(
                    command_registry
                        .commands()
                        .map(|(command, _)| command.to_string()),
                );
            }
        }

        // emit pending commands
        while let Ok(command) = self.event_receiver.try_recv() {
            match command {
                Event::Command(command) => net.queue_command(None, command),
                Event::Stop => net.shutdown(),
            }
        }
    }
}
