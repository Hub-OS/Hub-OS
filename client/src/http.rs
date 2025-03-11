pub async fn request(uri: &str) -> Option<Vec<u8>> {
    let mut response = surf::get(uri).await.ok()?;

    if !response.status().is_success() {
        log::error!(
            "Request {uri:?} failed:\n{:?}",
            response
                .body_string()
                .await
                .unwrap_or_else(|_| String::from("No reason provided"))
        );

        return None;
    }

    response.body_bytes().await.ok()
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
