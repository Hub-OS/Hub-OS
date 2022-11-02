#[macro_export]
macro_rules! cfg_desktop {
  ($($tt:tt)+) => {
    $crate::create_cfg_rules! { cfg(not(any(target_os = "android", target_arch = "wasm32"))), $($tt)+ }
  };
}

#[macro_export]
macro_rules! cfg_web {
  ($($tt:tt)+) => {
    $crate::create_cfg_rules! { cfg(target_arch = "wasm32"), $($tt)+ }
  };
}

#[macro_export]
macro_rules! cfg_desktop_and_web {
  ($($tt:tt)+) => {
    $crate::create_cfg_rules! { cfg(not(target_os = "android")), $($tt)+ }
  };
}

#[macro_export]
macro_rules! cfg_android {
  ($($tt:tt)+) => {
    $crate::create_cfg_rules! { cfg(target_os = "android"), $($tt)+ }
  };
}

#[macro_export]
macro_rules! cfg_native {
  ($($tt:tt)+) => {
    $crate::create_cfg_rules! { cfg(not(target_arch = "wasm32")), $($tt)+ }
  };
}

#[macro_export]
macro_rules! cfg_sdl {
  ($($tt:tt)+) => {
    $crate::create_cfg_rules! { cfg(feature = "sdl"), $($tt)+ }
  };
}

#[macro_export]
macro_rules! cfg_winit {
  ($($tt:tt)+) => {
    $crate::create_cfg_rules! { cfg(feature = "winit"), $($tt)+ }
  };
}

#[macro_export]
macro_rules! create_cfg_rules {
  ($meta:meta, $expr:expr) => {
    #[$meta]
    $expr
  };
  ($meta:meta, $($item:item)*) => {
    $(
      #[$meta]
      $item
    )+
  };
}
