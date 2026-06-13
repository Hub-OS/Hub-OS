use packets::address_parsing::uri_encode;
use serde::Deserialize;
#[derive(Default, Deserialize)]
pub struct UserResponse {
    #[serde(rename = "username")]
    pub name: String,
    pub avatar: Option<String>,
}

pub fn request_user(repo: &str, id: &str) -> impl Future<Output = Option<UserResponse>> + use<> {
    let encoded_id = uri_encode(id);
    let uri = format!("{repo}/api/users/{encoded_id}");

    async move { super::request_json(&uri).await }
}
