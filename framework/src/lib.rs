#![allow(clippy::let_and_return)]

pub mod async_task;
pub mod cfg;
pub mod graphics;
pub mod input;
pub mod logging;
pub mod math;
pub mod prelude;
pub mod util;

pub use wgpu;

mod common;

#[cfg(all(feature = "sdl", feature = "winit"))]
cfg_native! {
    compile_error!("features \"winit\" and \"sdl\" can not be enabled together. Add \"default-features = false\" to override the default features");
}

#[cfg(feature = "sdl")]
cfg_native! {
    mod sdl;
}

#[cfg(feature = "winit")]
cfg_native! {
    mod winit;
}

cfg_web! {
    mod winit;
}

/// Returns the name of the caller's crate
#[macro_export]
macro_rules! crate_name {
    () => {{
        // using module_path!() instead of env!("CARGO_PKG_NAME") to use the correct name within examples
        // must be used within a macro, otherwise module_path!() will return this crate's module and not the caller's module
        let module_name = module_path!();
        module_name
            .find(":")
            .map(|index| &module_name[..index])
            .unwrap_or(module_name)
    }};
}
