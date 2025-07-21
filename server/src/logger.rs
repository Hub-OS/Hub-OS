static LOGGER: Logger = Logger;

pub fn init() {
    log::set_logger(&LOGGER).unwrap();
    log::set_max_level(log::LevelFilter::Trace);
}

struct Logger;

impl log::Log for Logger {
    fn enabled(&self, metadata: &log::Metadata) -> bool {
        metadata.target().starts_with(env!("CARGO_PKG_NAME"))
    }

    fn log(&self, record: &log::Record) {
        use std::io::Write;
        use termcolor::{Color, ColorChoice, ColorSpec, StandardStream, WriteColor};

        let mut stdout = StandardStream::stdout(ColorChoice::Always);

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

            stdout.set_color(&color_spec).unwrap();
            writeln!(&mut stdout, "{msg}").unwrap();
            stdout.reset().unwrap();
        }
    }

    fn flush(&self) {}
}
