// Increment VERSION_ITERATION lib.rs if packets are added or modified

use super::structures::*;
use super::{VERSION_ID, VERSION_ITERATION};
use serde::{Deserialize, Serialize};
use strum::IntoStaticStr;

#[derive(Clone, PartialEq, Debug, Serialize, Deserialize, IntoStaticStr)]
pub enum ServerPacket {
    VersionInfo {
        version_id: String,
        version_iteration: u64,
    },
    Heartbeat,
    Authorize {
        address: String,
        data: Vec<u8>,
    },
    Login {
        actor_id: String,
        warp_in: bool,
        spawn_x: f32,
        spawn_y: f32,
        spawn_z: f32,
        spawn_direction: Direction,
    },
    CompleteConnection,
    TransferWarp,
    TransferStart,
    TransferComplete {
        warp_in: bool,
        direction: Direction,
    },
    TransferServer {
        address: String,
        data: String,
        warp_out: bool,
    },
    Kick {
        reason: String,
    },
    RemoveAsset {
        path: String,
    },
    AssetStreamStart {
        name: String,
        last_modified: u64,
        cache_to_disk: bool,
        data_type: AssetDataType,
        size: u64,
    },
    AssetStream {
        data: Vec<u8>,
    },
    Preload {
        asset_path: String,
        data_type: AssetDataType,
    },
    CustomEmotesPath {
        asset_path: String,
    },
    MapUpdate {
        map_path: String,
    },
    Health {
        health: i32,
    },
    BaseHealth {
        base_health: i32,
    },
    Emotion {
        emotion: u8,
    },
    Money {
        money: u32,
    },
    AddItem {
        id: String,
        name: String,
        description: String,
    },
    RemoveItem {
        id: String,
    },
    PlaySound {
        path: String,
    },
    ExcludeObject {
        id: u32,
    },
    IncludeObject {
        id: u32,
    },
    ExcludeActor {
        actor_id: String,
    },
    IncludeActor {
        actor_id: String,
    },
    MoveCamera {
        x: f32,
        y: f32,
        z: f32,
        hold_duration: f32,
    },
    SlideCamera {
        x: f32,
        y: f32,
        z: f32,
        duration: f32,
    },
    ShakeCamera {
        strength: f32,
        duration: f32,
    },
    FadeCamera {
        color: (u8, u8, u8, u8),
        duration: f32,
    },
    TrackWithCamera {
        actor_id: String,
    },
    EnableCameraControls {
        dist_x: f32,
        dist_y: f32,
    },
    UnlockCamera,
    LockInput,
    UnlockInput,
    Teleport {
        warp: bool,
        x: f32,
        y: f32,
        z: f32,
        direction: Direction,
    },
    Message {
        message: String,
        mug_texture_path: String,
        mug_animation_path: String,
    },
    Question {
        message: String,
        mug_texture_path: String,
        mug_animation_path: String,
    },
    Quiz {
        option_a: String,
        option_b: String,
        option_c: String,
        mug_texture_path: String,
        mug_animation_path: String,
    },
    Prompt {
        character_limit: u16,
        default_text: Option<String>,
    },
    TextBoxResponseAck,
    OpenBoard {
        topic: String,
        color: (u8, u8, u8),
        posts: Vec<BbsPost>,
        open_instantly: bool,
    },
    PrependPosts {
        reference: Option<String>,
        posts: Vec<BbsPost>,
    },
    AppendPosts {
        reference: Option<String>,
        posts: Vec<BbsPost>,
    },
    RemovePost {
        id: String,
    },
    PostSelectionAck,
    CloseBBS,
    ShopInventory {
        items: Vec<ShopItem>,
    },
    OpenShop {
        mug_texture_path: String,
        mug_animation_path: String,
    },
    OfferPackage {
        name: String,
        id: String,
        category: PackageCategory,
        package_path: String,
    },
    LoadPackage {
        category: PackageCategory,
        package_path: String,
    },
    ModWhitelist {
        whitelist_path: Option<String>,
    },
    ModBlacklist {
        blacklist_path: Option<String>,
    },
    InitiateEncounter {
        package_path: String,
        data: Option<String>,
        health: i32,
        base_health: i32,
    },
    InitiateNetplay {
        package_path: Option<String>,
        data: Option<String>,
        remote_players: Vec<RemotePlayerInfo>,
        health: i32,
        base_health: i32,
    },
    ActorConnected {
        actor_id: String,
        name: String,
        texture_path: String,
        animation_path: String,
        direction: Direction,
        x: f32,
        y: f32,
        z: f32,
        solid: bool,
        warp_in: bool,
        scale_x: f32,
        scale_y: f32,
        rotation: f32,
        map_color: (u8, u8, u8, u8),
        animation: Option<String>,
    },
    ActorDisconnected {
        actor_id: String,
        warp_out: bool,
    },
    ActorSetName {
        actor_id: String,
        name: String,
    },
    ActorMove {
        actor_id: String,
        x: f32,
        y: f32,
        z: f32,
        direction: Direction,
    },
    ActorSetAvatar {
        actor_id: String,
        texture_path: String,
        animation_path: String,
    },
    ActorEmote {
        actor_id: String,
        emote_id: u8,
        use_custom_emotes: bool,
    },
    ActorAnimate {
        actor_id: String,
        state: String,
        loop_animation: bool,
    },
    ActorPropertyKeyFrames {
        actor_id: String,
        keyframes: Vec<ActorKeyFrame>,
    },
    ActorMapColor {
        actor_id: String,
        color: (u8, u8, u8, u8),
    },
    SynchronizeUpdates,
    EndSynchronization,
}

impl ServerPacket {
    pub fn new_version_info() -> Self {
        ServerPacket::VersionInfo {
            version_id: VERSION_ID.to_string(),
            version_iteration: VERSION_ITERATION,
        }
    }

    pub fn create_asset_stream<'a>(
        max_payload_size: u16,
        name: &str,
        asset: &'a impl AssetTrait,
    ) -> impl Iterator<Item = ServerPacket> + 'a {
        // header + packet type + data size
        const HEADER_SIZE: usize = 56 + 2 + 2;

        let bytes = match &asset.data() {
            AssetData::Text(data) => data.as_bytes(),
            AssetData::CompressedText(data) => data,
            AssetData::Texture(data) => data,
            AssetData::Audio(data) => data,
            AssetData::Data(data) => data,
        };

        let initial_packet = ServerPacket::AssetStreamStart {
            name: name.to_string(),
            last_modified: asset.last_modified(),
            cache_to_disk: asset.cache_to_disk(),
            data_type: asset.data().data_type(),
            size: bytes.len() as u64,
        };

        let data_packets = bytes
            .chunks(max_payload_size as usize - HEADER_SIZE)
            .map(|bytes| ServerPacket::AssetStream {
                data: bytes.to_vec(),
            });

        std::iter::once(initial_packet).chain(data_packets)
    }
}
