use std::sync::Mutex;

static LOGGER: Logger = Logger;
static PRINTER: Mutex<Option<Box<dyn Fn(String) + Send + Sync>>> = Mutex::new(None);

pub fn init() {
    log::set_logger(&LOGGER).unwrap();
    log::set_max_level(log::LevelFilter::Trace);
}

pub fn set_printer(callback: impl Fn(String) + Send + Sync + 'static) {
    let mut cell = PRINTER.lock().unwrap();
    *cell = Some(Box::new(callback));
}

struct Logger;

impl log::Log for Logger {
    fn enabled(&self, metadata: &log::Metadata) -> bool {
        metadata.target().starts_with(env!("CARGO_PKG_NAME"))
    }

    fn log(&self, record: &log::Record) {
        use std::io::Write;
        use termcolor::{Color, ColorChoice, ColorSpec, WriteColor};

        let mut buffer = termcolor::BufferWriter::stdout(ColorChoice::Always).buffer();

        if self.enabled(record.metadata()) {
            let msg = format!("{}", record.args());
            let mut color_spec = ColorSpec::new();

            match record.level() {
                log::Level::Error => {
                    color_spec.set_fg(Some(Color::Red));
                }
                log::Level::Warn => {
                    color_spec.set_fg(Some(Color::Yellow));
                }
                log::Level::Info => {}
                log::Level::Debug => {
                    color_spec.set_dimmed(true);
                }
                log::Level::Trace => {
                    color_spec.set_dimmed(true);
                }
            };

            buffer.set_color(&color_spec).unwrap();
            buffer.write_all(msg.as_bytes()).unwrap();
            buffer.reset().unwrap();

            let output = String::from_utf8_lossy(buffer.as_slice());

            let guard = PRINTER.lock().unwrap();

            if let Some(print) = &*guard {
                print(output.to_string());
            } else {
                println!("{output}");
            }
        }
    }

    fn flush(&self) {}
}
