use packets::address_parsing::uri_encode;
use packets::structures::{FileHash, PackageCategory, PackageId};

pub fn request_package_hashes<P>(
    repo: &str,
    // todo: update to an iterator when this is fixed: "currently, all type parameters are required to be mentioned in the precise captures list"
    package_ids: Vec<&PackageId>,
    progress_callback: P,
) -> impl Future<Output = Vec<(PackageCategory, PackageId, FileHash)>> + use<P>
where
    P: FnMut(usize, usize) + 'static,
{
    let id_strings = package_ids
        .into_iter()
        .map(|id| uri_encode(id.as_str()))
        .collect();
    request_package_hashes_inner(repo, id_strings, progress_callback)
}

fn request_package_hashes_inner<P>(
    repo: &str,
    package_ids: Vec<String>,
    mut progress_callback: P,
) -> impl Future<Output = Vec<(PackageCategory, PackageId, FileHash)>> + use<P>
where
    P: FnMut(usize, usize) + 'static,
{
    let repo = repo.to_string();

    async move {
        let mut hash_list = Vec::new();

        // nginx has an 8k header limit
        const ID_BYTE_LIMIT: usize = 7000;
        const ID_JOIN_STR: &str = "&id=";

        // resolve id chunks
        let mut id_bytes_remaining = ID_BYTE_LIMIT;

        let id_chunk_iter = package_ids.chunk_by(|id_a, id_b| {
            let bytes_required = id_a.len() + ID_JOIN_STR.len() + id_b.len();

            if id_bytes_remaining <= bytes_required {
                id_bytes_remaining = ID_BYTE_LIMIT;
                return false;
            }

            id_bytes_remaining = id_bytes_remaining.saturating_sub(bytes_required - id_b.len());

            true
        });

        // request hashes
        let mut ids_requested = 0;

        for chunk in id_chunk_iter {
            ids_requested += chunk.len();

            progress_callback(ids_requested, package_ids.len());

            let uri = format!("{repo}/api/mods/hashes?id={}", chunk.join(ID_JOIN_STR));

            let Some(json) = crate::requests::request_json::<serde_json::Value>(&uri).await else {
                continue;
            };

            let Some(arr) = json.as_array() else {
                continue;
            };

            let transform_iter = arr
                .iter()
                .flat_map(|value| {
                    Some((
                        value.get("category")?.as_str()?,
                        value.get("id")?.as_str()?,
                        value.get("hash")?.as_str()?,
                    ))
                })
                .flat_map(|(category, id, hash)| {
                    Some((
                        PackageCategory::from(category),
                        PackageId::from(id),
                        FileHash::from_hex(hash)?,
                    ))
                });

            hash_list.extend(transform_iter);
        }

        hash_list
    }
}
