use super::job_promise::{JobPromise, PromiseValue};

pub fn write_file(path: String, content: &[u8]) -> JobPromise {
    let promise = JobPromise::new();
    let mut thread_promise = promise.clone();

    // own content for thread
    let content = content.to_vec();

    async_std::task::spawn(async move {
        use async_std::fs::write;

        let success = write(path, content).await.is_ok();

        thread_promise.set_value(PromiseValue::Success(success));
    });

    promise
}
