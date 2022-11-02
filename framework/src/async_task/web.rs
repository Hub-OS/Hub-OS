use js_sys::Promise;
use std::time::Duration;
use wasm_bindgen_futures::JsFuture;

pub async fn sleep(duration: Duration) {
  let window = web_sys::window().unwrap();

  let promise = Promise::new(&mut |resolve, _reject| {
    window
      .set_timeout_with_callback_and_timeout_and_arguments_0(&resolve, duration.as_millis() as i32)
      .unwrap();
  });

  let _ = JsFuture::from(promise).await;
}
