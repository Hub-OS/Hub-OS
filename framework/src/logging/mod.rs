pub use log::Level as LogLevel;
pub use log::LevelFilter as LogLevelFilter;
pub use log::{debug, error, info, trace, warn};
pub mod default_logger;

#[derive(Clone)]
pub struct LogRecord {
    pub level: LogLevel,
    pub target: String,
    pub message: String,
}

crate::cfg_web! {
  mod web_logger;
}

crate::cfg_native! {
  mod native_logger;
}
