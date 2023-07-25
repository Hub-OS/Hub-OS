pub async fn request(uri: &str) -> Option<Vec<u8>> {
    let mut response = surf::get(uri).await.ok()?;

    if !response.status().is_success() {
        return None;
    }

    response.body_bytes().await.ok()
}

pub async fn request_json(uri: &str) -> Option<serde_json::Value> {
    let response_vec = request(uri).await?;
    serde_json::from_slice::<serde_json::Value>(&response_vec).ok()
}
