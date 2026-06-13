use crate::bindable::{CardClass, Element};
use crate::saves::BlockShape;
use packets::address_parsing::uri_encode;
use packets::structures::{BlockColor, FileHash, PackageId};
use serde::Deserialize;
use serde_with::{DefaultOnError, serde_as};

fn yes() -> bool {
    true
}

#[serde_as]
#[derive(Default, Deserialize)]
#[serde(default)]
pub struct PackageResponseMeta {
    #[serde_as(deserialize_as = "DefaultOnError")]
    pub category: String,
    #[serde_as(deserialize_as = "DefaultOnError")]
    pub id: PackageId,
    #[serde_as(deserialize_as = "DefaultOnError")]
    pub past_ids: Vec<PackageId>,
    #[serde_as(deserialize_as = "DefaultOnError")]
    pub name: String,
    #[serde_as(deserialize_as = "DefaultOnError")]
    pub long_name: String,
    #[serde_as(deserialize_as = "DefaultOnError")]
    pub description: String,
    #[serde_as(deserialize_as = "DefaultOnError")]
    pub long_description: String,

    // players
    #[serde_as(deserialize_as = "DefaultOnError")]
    pub health: i32,

    // players and cards
    #[serde(deserialize_with = "deserialize_element")]
    pub element: Element,
    #[serde(deserialize_with = "deserialize_element")]
    pub secondary_element: Element,

    // cards
    #[serde(deserialize_with = "deserialize_card_class")]
    pub card_class: CardClass,
    #[serde_as(deserialize_as = "DefaultOnError")]
    pub codes: Vec<String>,
    #[serde_as(deserialize_as = "DefaultOnError")]
    pub damage: i32,
    #[serde_as(deserialize_as = "DefaultOnError")]
    #[serde(default = "yes")]
    pub can_boost: bool,

    // augments
    #[serde_as(deserialize_as = "DefaultOnError")]
    pub slot: String,
    #[serde_as(deserialize_as = "DefaultOnError")]
    pub flat: bool,
    #[serde(deserialize_with = "deserialize_block_colors")]
    pub colors: Vec<BlockColor>,
    #[serde(deserialize_with = "deserialize_block_shape")]
    pub shape: Option<BlockShape>,

    // recordings
    #[serde_as(deserialize_as = "DefaultOnError")]
    pub recording_path: String,
}

#[serde_with::apply(_ => #[serde_as(deserialize_as = "DefaultOnError")])]
#[self::serde_as]
#[derive(Default, Deserialize)]
#[serde(default)]
pub struct PackageResponseDependencies {
    pub augments: Vec<PackageId>,
    pub encounters: Vec<PackageId>,
    pub characters: Vec<PackageId>,
    pub libraries: Vec<PackageId>,
    pub statuses: Vec<PackageId>,
    pub cards: Vec<PackageId>,
    pub tile_states: Vec<PackageId>,
}

#[serde_as]
#[derive(Deserialize)]
pub struct PackageResponse {
    pub package: PackageResponseMeta,
    #[serde_as(deserialize_as = "DefaultOnError")]
    #[serde(default)]
    pub dependencies: PackageResponseDependencies,
    pub creator: String,
    #[serde(default, deserialize_with = "deserialize_hash")]
    pub hash: FileHash,
}

pub fn deserialize_element<'de, D>(deserializer: D) -> Result<Element, D::Error>
where
    D: serde::Deserializer<'de>,
{
    let Ok(s) = String::deserialize(deserializer) else {
        return Ok(Default::default());
    };

    Ok(Element::from(s))
}

pub fn deserialize_card_class<'de, D>(deserializer: D) -> Result<CardClass, D::Error>
where
    D: serde::Deserializer<'de>,
{
    let Ok(class_str) = String::deserialize(deserializer) else {
        return Ok(Default::default());
    };

    Ok(CardClass::from(class_str))
}

pub fn deserialize_block_colors<'de, D>(deserializer: D) -> Result<Vec<BlockColor>, D::Error>
where
    D: serde::Deserializer<'de>,
{
    let Ok(list) = <Vec<String>>::deserialize(deserializer) else {
        return Ok(Default::default());
    };

    Ok(list.into_iter().map(BlockColor::from).collect())
}

pub fn deserialize_block_shape<'de, D>(deserializer: D) -> Result<Option<BlockShape>, D::Error>
where
    D: serde::Deserializer<'de>,
{
    let Ok(shape) = <Vec<Vec<u8>>>::deserialize(deserializer) else {
        return Ok(Default::default());
    };

    Ok(BlockShape::try_from(shape).ok())
}

pub fn deserialize_hash<'de, D>(deserializer: D) -> Result<FileHash, D::Error>
where
    D: serde::Deserializer<'de>,
{
    let hash_str = String::deserialize(deserializer)?;

    Ok(FileHash::from_hex(&hash_str).unwrap_or_default())
}

pub fn request_package_meta(
    repo: &str,
    id: &PackageId,
) -> impl Future<Output = Option<PackageResponse>> + use<> {
    let encoded_id = uri_encode(id.as_str());
    let uri = format!("{repo}/api/mods/{encoded_id}/meta");

    async move { super::request_json(&uri).await }
}
