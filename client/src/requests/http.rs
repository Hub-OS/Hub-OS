pub async fn request(uri: &str) -> Option<Vec<u8>> {
    // our uri encoder preserves spaces
    let uri = uri.replace(' ', "%20");

    let response = smol::unblock(move || {
        minreq::get(uri.clone())
            .send()
            .map_err(|err| Some(err.to_string()))
            .and_then(|response| {
                if response.status_code == 200 {
                    Ok(response.into_bytes())
                } else {
                    Err(response.as_str().map(|s| s.to_string()).ok())
                }
            })
            .inspect_err(|err| {
                log::error!(
                    "Request {uri:?} failed:\n{:?}",
                    err.as_ref()
                        .map(|s| s.as_str())
                        .unwrap_or("No reason provided")
                );
            })
            .ok()
    });

    response.await
}

pub async fn request_json<T: serde::de::DeserializeOwned>(uri: &str) -> Option<T> {
    let response_vec = request(uri).await?;
    let response_string = String::from_utf8_lossy(&response_vec);
    // let cursor = std::io::Cursor::new(&response_string);
    let mut deserializer = serde_json::Deserializer::from_str(&response_string);

    match serde_path_to_error::deserialize(&mut deserializer) {
        Ok(value) => Some(value),
        Err(err) => {
            log::error!(
                "Received invalid response from {uri:?}, {err}:\n{:?}",
                // err.path(),
                String::from_utf8_lossy(&response_vec)
            );
            None
        }
    }
}
