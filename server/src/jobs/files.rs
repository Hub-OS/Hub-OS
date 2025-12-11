use super::job_promise::{JobPromise, PromiseValue};

pub fn read_file(path: String) -> JobPromise {
    let promise = JobPromise::new();
    let mut thread_promise = promise.clone();

    smol::spawn(async move {
        use smol::fs::read;

        let contents = read(path).await.ok().unwrap_or_default();

        thread_promise.set_value(PromiseValue::Bytes(contents));
    })
    .detach();

    promise
}

pub fn write_file(path: String, content: &[u8]) -> JobPromise {
    let promise = JobPromise::new();
    let mut thread_promise = promise.clone();

    // own content for thread
    let content = content.to_vec();

    smol::spawn(async move {
        use smol::fs::write;

        let success = write(path, content).await.is_ok();

        thread_promise.set_value(PromiseValue::Success(success));
    })
    .detach();

    promise
}

pub fn ensure_folder(path: String) -> JobPromise {
    let promise = JobPromise::new();
    let mut thread_promise = promise.clone();

    smol::spawn(async move {
        use smol::fs::create_dir_all;

        let _ = create_dir_all(path).await;

        thread_promise.set_value(PromiseValue::None);
    })
    .detach();

    promise
}
