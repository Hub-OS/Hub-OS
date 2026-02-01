use super::job_promise::{JobPromise, PromiseValue};

pub struct HttpResponse {
    pub status: u16,
    pub body: Vec<u8>,
    pub headers: Vec<(String, String)>,
}

pub fn web_request(
    url: String,
    method: String,
    headers: Vec<(String, String)>,
    body: Option<Vec<u8>>,
) -> JobPromise {
    let promise = JobPromise::new();
    let mut thread_promise = promise.clone();

    smol::unblock(move || {
        let mut request = minreq::Request::new(minreq::Method::Custom(method), url)
            .with_follow_redirects(true)
            .with_headers(headers);

        if let Some(body) = body {
            request = request.with_body(body);
        }

        let mut response = match request.send() {
            Ok(response) => response,
            Err(err) => {
                log::warn!("{err}");
                thread_promise.set_value(PromiseValue::None);
                return;
            }
        };

        thread_promise.set_value(PromiseValue::HttpResponse(HttpResponse {
            status: response.status_code as u16,
            headers: std::mem::take(&mut response.headers).into_iter().collect(),
            body: response.into_bytes(),
        }));
    })
    .detach();

    promise
}
