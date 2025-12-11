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
    use futures::io::AsyncReadExt;

    let promise = JobPromise::new();
    let mut thread_promise = promise.clone();

    smol::spawn(async move {
        let response = match web_request_internal(url, method, headers, body).await {
            Ok(response) => response,
            Err(err) => {
                log::warn!("{err}");
                thread_promise.set_value(PromiseValue::None);
                return;
            }
        };

        let status: u16 = response.status().into();
        let mut headers = Vec::new();

        for (name, value) in response.headers().iter() {
            headers.push((
                String::from(name.as_str()),
                String::from_utf8_lossy(value.as_bytes()).into_owned(),
            ));
        }

        let mut body = Vec::new();

        match response.into_body().read_to_end(&mut body).await {
            Ok(body) => body,
            Err(err) => {
                log::warn!("{err}");

                thread_promise.set_value(PromiseValue::None);
                return;
            }
        };

        thread_promise.set_value(PromiseValue::HttpResponse(HttpResponse {
            status,
            body,
            headers,
        }));
    })
    .detach();

    promise
}

pub(super) async fn web_request_internal(
    url: String,
    method: String,
    headers: Vec<(String, String)>,
    body: Option<Vec<u8>>,
) -> Result<isahc::Response<isahc::AsyncBody>, Box<dyn std::error::Error>> {
    use isahc::config::{Configurable, RedirectPolicy};

    let mut request_builder = isahc::Request::builder()
        .uri(url)
        .method(method.as_str())
        .redirect_policy(RedirectPolicy::Follow);

    for (key, value) in headers {
        request_builder = request_builder.header(key.as_str(), value.as_str());
    }

    let request = match body {
        Some(data) => request_builder.body(data)?,
        None => request_builder.body(Vec::new())?,
    };

    // handling response
    let response = isahc::send_async(request).await?;

    Ok(response)
}
