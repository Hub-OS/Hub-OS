use futures::AsyncReadExt;

pub async fn request(uri: &str) -> Option<Vec<u8>> {
    let response = isahc::get_async(uri).await.ok()?;

    if !response.status().is_success() {
        return None;
    }

    let mut body = response.into_body();
    let mut response_vec = Vec::new();

    if body.read_to_end(&mut response_vec).await.is_err() {
        return None;
    }

    Some(response_vec)
}

pub async fn request_json(uri: &str) -> Option<serde_json::Value> {
    let response_vec = request(uri).await?;
    serde_json::from_slice::<serde_json::Value>(&response_vec).ok()
}
