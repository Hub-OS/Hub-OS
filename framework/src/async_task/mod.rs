mod async_task;
mod sync_result_async_error;

pub use self::async_task::*;
pub use sync_result_async_error::SyncResultAsyncError;

// no wasm support https://github.com/async-rs/async-std/issues/220
crate::cfg_native! {
  mod native;

  pub use self::native::*;
}

crate::cfg_web! {
  mod web;

  pub use self::web::*;
}

pub use pollster::block_on;
