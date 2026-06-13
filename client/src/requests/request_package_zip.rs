use packets::address_parsing::uri_encode;
use packets::structures::PackageId;

pub fn request_package_zip(
    repo: &str,
    id: &PackageId,
) -> impl Future<Output = Option<Vec<u8>>> + use<> {
    let encoded_id = uri_encode(id.as_str());
    let uri = format!("{repo}/api/mods/{encoded_id}");

    async move { super::request(&uri).await }
}
