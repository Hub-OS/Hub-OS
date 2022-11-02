use wasm_bindgen::JsValue;

pub struct DefaultLogger {
    listeners: Vec<Box<dyn Fn(LogRecord) + Send + Sync>>,
    default_level_filter: log::LevelFilter,
    crate_level_filters: Vec<(String, log::LevelFilter)>,
}

impl DefaultLogger {
    pub fn new() -> Self {
        Self {
            listeners: Vec::new(),
            default_level_filter: log::LevelFilter::Trace,
            crate_level_filters: Vec::new(),
        }
    }

    pub fn with_global_level_filter(mut self, level: log::LevelFilter) -> Self {
        self.default_level_filter = level;

        self
    }

    pub fn with_crate_level_filter(mut self, crate_name: &str, level: log::LevelFilter) -> Self {
        self.crate_level_filters
            .push((crate_name.replace('-', "_"), level));

        self
    }

    pub fn with_listener(mut self, listener: impl Fn(LogRecord) + Send + Sync + 'static) -> Self {
        self.listeners.push(Box::new(listener));
        self
    }

    pub fn init(self) -> Result<(), log::SetLoggerError> {
        std::panic::set_hook(Box::new(console_error_panic_hook::hook));

        // allow everything, we'll filter logs within the logger
        log::set_max_level(log::LevelFilter::Trace);
        log::set_boxed_logger(Box::new(self))
    }
}

impl Default for DefaultLogger {
    fn default() -> Self {
        Self::new()
    }
}

impl log::Log for DefaultLogger {
    fn enabled(&self, metadata: &log::Metadata) -> bool {
        let filter_tuple = self
            .crate_level_filters
            .iter()
            .find(|(name, _)| metadata.target().starts_with(name));

        if let Some((_, level)) = filter_tuple {
            metadata.level() <= *level
        } else {
            metadata.level() <= self.default_level_filter
        }
    }

    fn log(&self, record: &log::Record) {
        if !self.listeners.is_empty() {
            let record = LogRecord {
                target: record.target().to_string(),
                level: record.level(),
                message: format!("{}", record.args()),
            };

            for listener in &self.listeners {
                listener(record.clone());
            }
        }

        if self.enabled(record.metadata()) {
            let message = format!(
                "%c{}%c [{}] {}",
                record.level(),
                record.target(),
                record.args()
            );
            let log_level_style = get_style_str(record.level());

            let console_args = js_sys::Array::of3(
                &JsValue::from(message),
                &JsValue::from(log_level_style),
                &JsValue::from("color: inherit"),
            );

            match record.level() {
                log::Level::Error => web_sys::console::error(&console_args),
                log::Level::Warn => web_sys::console::warn(&console_args),
                log::Level::Info => web_sys::console::info(&console_args),
                log::Level::Debug => web_sys::console::log(&console_args),
                log::Level::Trace => web_sys::console::debug(&console_args),
            };
        }
    }

    fn flush(&self) {}
}

fn get_style_str(level: log::Level) -> &'static str {
    match level {
        log::Level::Error => "color: inherit",
        log::Level::Warn => "color: orange",
        log::Level::Info => "color: inherit",
        log::Level::Debug => "color: #009900",
        log::Level::Trace => "color: inherit",
    }
}
