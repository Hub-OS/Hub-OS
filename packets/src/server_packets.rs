// Increment VERSION_ITERATION lib.rs if packets are added or modified

use super::structures::*;
use super::{VERSION_ID, VERSION_ITERATION};
use serde::{Deserialize, Serialize};
use strum::IntoStaticStr;

#[derive(Clone, PartialEq, Default, Debug, Serialize, Deserialize)]
pub struct ReferOptions {
    pub unless_installed: bool,
}

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
    PackageList {
        packages: Vec<(String, PackageCategory, PackageId, FileHash)>,
    },
    Login {
        actor_id: ActorId,
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
        hash: FileHash,
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
        animation_path: String,
        texture_path: String,
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
        emotion: Emotion,
    },
    Money {
        money: u32,
    },
    RegisterItem {
        id: String,
        item_definition: ItemDefinition,
    },
    AddItem {
        id: String,
        count: isize,
    },
    AddCard {
        package_id: PackageId,
        code: String,
        count: isize,
    },
    AddBlock {
        package_id: PackageId,
        color: BlockColor,
        count: isize,
    },
    EnablePlayableCharacter {
        package_id: PackageId,
        enabled: bool,
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
        actor_id: ActorId,
    },
    IncludeActor {
        actor_id: ActorId,
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
        actor_id: ActorId,
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
    HideHud,
    ShowHud,
    Message {
        message: String,
        textbox_options: TextboxOptions,
    },
    AutoMessage {
        message: String,
        close_delay: f32,
        textbox_options: TextboxOptions,
    },
    Question {
        message: String,
        textbox_options: TextboxOptions,
    },
    Quiz {
        option_a: String,
        option_b: String,
        option_c: String,
        textbox_options: TextboxOptions,
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
    SelectionAck,
    CloseBoard,
    OpenShop {
        textbox_options: TextboxOptions,
    },
    PrependShopItems {
        reference: Option<String>,
        items: Vec<ShopItem>,
    },
    AppendShopItems {
        reference: Option<String>,
        items: Vec<ShopItem>,
    },
    ShopMessage {
        message: String,
    },
    UpdateShopItem {
        item: ShopItem,
    },
    RemoveShopItem {
        id: String,
    },
    ReferLink {
        address: String,
    },
    ReferServer {
        name: String,
        address: String,
    },
    ReferPackage {
        package_id: PackageId,
        options: ReferOptions,
    },
    OfferPackage {
        name: String,
        id: PackageId,
        category: PackageCategory,
        package_path: String,
    },
    LoadPackage {
        category: PackageCategory,
        package_path: String,
    },
    Restrictions {
        restrictions_path: Option<String>,
    },
    InitiateEncounter {
        battle_id: BattleId,
        package_path: String,
        data: Option<String>,
    },
    InitiateNetplay {
        battle_id: BattleId,
        package_path: Option<String>,
        data: Option<String>,
        seed: u64,
        remote_players: Vec<RemotePlayerInfo>,
    },
    BattleMessage {
        battle_id: BattleId,
        data: String,
    },
    ActorConnected {
        actor_id: ActorId,
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
        loop_animation: bool,
    },
    ActorDisconnected {
        actor_id: ActorId,
        warp_out: bool,
    },
    ActorSetName {
        actor_id: ActorId,
        name: String,
    },
    ActorMove {
        actor_id: ActorId,
        x: f32,
        y: f32,
        z: f32,
        direction: Direction,
    },
    ActorSetAvatar {
        actor_id: ActorId,
        texture_path: String,
        animation_path: String,
    },
    ActorEmote {
        actor_id: ActorId,
        emote_id: String,
    },
    ActorAnimate {
        actor_id: ActorId,
        state: String,
        loop_animation: bool,
    },
    ActorPropertyKeyFrames {
        actor_id: ActorId,
        keyframes: Vec<ActorKeyFrame>,
    },
    ActorMapColor {
        actor_id: ActorId,
        color: (u8, u8, u8, u8),
    },
    SpriteCreated {
        sprite_id: SpriteId,
        attachment: SpriteAttachment,
        definition: SpriteDefinition,
    },
    SpriteAnimate {
        sprite_id: SpriteId,
        state: String,
        loop_animation: bool,
    },
    SpriteDeleted {
        sprite_id: SpriteId,
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
            hash: asset.hash(),
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
