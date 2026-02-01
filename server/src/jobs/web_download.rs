use super::job_promise::{JobPromise, PromiseValue};

pub fn web_download(
    destination: String,
    url: String,
    method: String,
    headers: Vec<(String, String)>,
    body: Option<Vec<u8>>,
) -> JobPromise {
    let promise = JobPromise::new();
    let mut thread_promise = promise.clone();

    let future = smol::unblock(move || {
        // make request
        let mut request = minreq::Request::new(minreq::Method::Custom(method), url)
            .with_follow_redirects(true)
            .with_headers(headers);

        if let Some(body) = body {
            request = request.with_body(body);
        }

        let response = match request.send_lazy() {
            Ok(response) => response,
            Err(_) => {
                return false;
            }
        };

        // read response to file
        use std::io::{BufRead, Write};

        let mut buf_reader = std::io::BufReader::new(response);
        let mut buf_writer = match std::fs::File::create(&destination) {
            Ok(file) => std::io::BufWriter::new(file),
            Err(err) => {
                log::warn!("{err}");
                return false;
            }
        };

        let mut length = 1;

        while length > 0 {
            let buffer = match buf_reader.fill_buf() {
                Ok(buffer) => buffer,
                Err(err) => {
                    let _ = std::fs::remove_file(destination);
                    log::warn!("{err}");
                    return false;
                }
            };

            length = buffer.len();

            if let Err(err) = buf_writer.write_all(buffer) {
                let _ = std::fs::remove_file(destination);
                log::warn!("{err}");
                return false;
            };

            buf_reader.consume(length);
        }

        if let Err(err) = buf_writer.flush() {
            let _ = std::fs::remove_file(destination);
            log::warn!("{err}");
            return false;
        };

        true
    });

    smol::spawn(async move {
        let success = future.await;
        thread_promise.set_value(PromiseValue::Success(success));
    })
    .detach();

    promise
}
