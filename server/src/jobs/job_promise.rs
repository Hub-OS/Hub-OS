use super::web_request::HttpResponse;
use std::sync::{Arc, Mutex};

pub enum PromiseValue {
    HttpResponse(HttpResponse),
    Bytes(Vec<u8>),
    Success(bool),
    ServerPolled {},
    None,
}

#[derive(Clone)]
pub struct JobPromise {
    internal_promise: Arc<Mutex<Option<PromiseValue>>>,
}

impl JobPromise {
    pub fn new() -> JobPromise {
        JobPromise {
            internal_promise: Arc::new(Mutex::new(None)),
        }
    }

    pub fn is_pending(&self) -> bool {
        if let Ok(lock) = self.internal_promise.try_lock() {
            let promise_option = &*lock;

            return matches!(promise_option, None);
        }

        true
    }

    pub fn get_value(&mut self) -> Option<PromiseValue> {
        if let Ok(mut lock) = self.internal_promise.try_lock() {
            return (*lock).take();
        }

        None
    }

    pub fn set_value(&mut self, value: PromiseValue) {
        let mut lock = self.internal_promise.lock().unwrap();

        *lock = Some(value);
    }
}
