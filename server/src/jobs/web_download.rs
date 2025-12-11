use super::job_promise::{JobPromise, PromiseValue};
use super::web_request::web_request_internal;

pub fn web_download(
    destination: String,
    url: String,
    method: String,
    headers: Vec<(String, String)>,
    body: Option<Vec<u8>>,
) -> JobPromise {
    let promise = JobPromise::new();
    let mut thread_promise = promise.clone();

    smol::spawn(async move {
        let response = match web_request_internal(url, method, headers, body).await {
            Ok(response) => response,
            Err(err) => {
                log::warn!("{err}");
                thread_promise.set_value(PromiseValue::Success(false));
                return;
            }
        };

        // writing to file
        if let Err(err) = save_response(destination, response).await {
            log::warn!("{err}");
            thread_promise.set_value(PromiseValue::Success(false));
            return;
        }

        thread_promise.set_value(PromiseValue::Success(true));
    })
    .detach();

    promise
}

async fn save_response(
    destination: String,
    response: isahc::Response<isahc::AsyncBody>,
) -> Result<(), Box<dyn std::error::Error>> {
    use futures::io::BufReader;
    use futures::{AsyncBufReadExt, AsyncWriteExt};
    use smol::fs::File;
    use smol::io::BufWriter;

    let file = File::create(destination).await?;

    let mut buf_reader = BufReader::new(response.into_body());
    let mut buf_writer = BufWriter::new(file);

    let mut length = 1;

    while length > 0 {
        let buffer = buf_reader.fill_buf().await?;
        length = buffer.len();

        buf_writer.write_all(buffer).await?;

        buf_reader.consume_unpin(length);
    }

    buf_writer.flush().await?;

    Ok(())
}
