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

pub async fn request_json(uri: &str) -> Option<serde_json::Value> {
    let response_vec = request(uri).await?;
    let result = serde_json::from_slice::<serde_json::Value>(&response_vec);

    match result {
        Ok(value) => Some(value),
        Err(_) => {
            log::error!(
                "Received invalid JSON from {uri:?}:\n{:?}",
                String::from_utf8_lossy(&response_vec)
            );
            None
        }
    }
}
