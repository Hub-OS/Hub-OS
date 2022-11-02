crate::cfg_web! {
    pub use super::web_logger::DefaultLogger;
}

crate::cfg_native! {
    pub use super::native_logger::DefaultLogger;
}

#[macro_export]
macro_rules! init {
    () => {{
        use $crate::logging::{default_logger::DefaultLogger, LogLevelFilter};

        DefaultLogger::new()
            .with_global_level_filter(LogLevelFilter::Warn)
            .with_crate_level_filter($crate::crate_name!(), LogLevelFilter::Trace)
            .init()
            .unwrap();
    }};
}

#[macro_export]
macro_rules! init_with_listener {
    ($listener:expr) => {{
        use $crate::logging::{default_logger::DefaultLogger, LogLevelFilter};

        DefaultLogger::new()
            .with_global_level_filter(LogLevelFilter::Warn)
            .with_crate_level_filter($crate::crate_name!(), LogLevelFilter::Trace)
            .with_listener($listener)
            .init()
            .unwrap();
    }};
}

pub use init;
pub use init_with_listener;
