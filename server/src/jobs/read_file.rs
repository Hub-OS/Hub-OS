use super::job_promise::{JobPromise, PromiseValue};

pub fn read_file(path: String) -> JobPromise {
    let promise = JobPromise::new();
    let mut thread_promise = promise.clone();

    async_std::task::spawn(async move {
        use async_std::fs::read;

        let contents = read(path).await.ok().unwrap_or_default();

        thread_promise.set_value(PromiseValue::Bytes(contents));
    });

    promise
}
