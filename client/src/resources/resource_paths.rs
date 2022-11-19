pub struct ResourcePaths;

#[cfg(not(target_os = "android"))]
lazy_static::lazy_static! {
    static ref GAME_PATH: String = ResourcePaths::clean_folder(
        &std::env::current_dir()
            .unwrap_or_default()
            .to_string_lossy()
    );
}

#[cfg(target_os = "android")]
lazy_static::lazy_static! {
    static ref GAME_PATH: String = ResourcePaths::clean_folder(
        &ndk_glue::native_activity()
            .external_data_path()
            .to_string_lossy()
    );
}

impl ResourcePaths {
    pub const CACHE_FOLDER: &str = "cache/";
    pub const MOD_CACHE_FOLDER: &str = "cache/local_packages/";
    pub const VIRTUAL_PREFIX: &str = "/virtual/";
    pub const SEPARATOR: &str = "/";

    // Music
    pub const BATTLE_MUSIC: &str = "resources/music/battle.ogg";

    // SFX
    pub const CURSOR_MOVE_SFX: &str = "resources/sfx/cursor_move.ogg";
    pub const CURSOR_SELECT_SFX: &str = "resources/sfx/cursor_select.ogg";
    pub const CURSOR_CANCEL_SFX: &str = "resources/sfx/cursor_cancel.ogg";
    pub const CURSOR_ERROR_SFX: &str = "resources/sfx/cursor_error.ogg";
    pub const MENU_OPEN_SFX: &str = "resources/sfx/menu_open.ogg";
    pub const MENU_CLOSE_SFX: &str = "resources/sfx/menu_close.ogg";
    pub const PAGE_TURN_SFX: &str = "resources/sfx/cursor_move.ogg";
    pub const TEXT_BLIP_SFX: &str = "resources/sfx/text.ogg";
    pub const WARP_SFX: &str = "resources/sfx/warp.ogg";
    pub const APPEAR_SFX: &str = "resources/sfx/appear.ogg";
    pub const CARD_SELECT_OPEN_SFX: &str = "resources/sfx/card_select_open.ogg";
    pub const CARD_SELECT_CONFIRM_SFX: &str = "resources/sfx/card_select_confirm.ogg";
    pub const TURN_GAUGE_SFX: &str = "resources/sfx/turn_gauge_full.ogg";
    pub const TIME_FREEZE_SFX: &str = "resources/sfx/time_freeze.ogg";
    pub const TILE_BREAK_SFX: &str = "resources/sfx/tile_break.ogg";
    pub const TRAP_SFX: &str = "resources/sfx/trap.ogg";
    pub const ATTACK_CHARGING_SFX: &str = "resources/sfx/attack_charging.ogg";
    pub const ATTACK_CHARGED_SFX: &str = "resources/sfx/attack_charged.ogg";
    pub const PLAYER_DELETED_SFX: &str = "resources/sfx/player_deleted.ogg";
    pub const HURT_SFX: &str = "resources/sfx/hurt.ogg";
    pub const EXPLODE_SFX: &str = "resources/sfx/explode.ogg";

    // General
    pub const BLANK: &str = "";
    pub const WHITE_PIXEL: &str = "resources/scenes/shared/white_pixel.png";
    pub const FONTS: &str = "resources/fonts/fonts_compressed.png";
    pub const FONTS_ANIMATION: &str = "resources/fonts/fonts_compressed.animation";
    pub const PAGE_ARROW: &str = "resources/scenes/shared/page_arrow.png";
    pub const PAGE_ARROW_ANIMATION: &str = "resources/scenes/shared/page_arrow.animation";
    pub const SELECT_CURSOR: &str = "resources/scenes/shared/select_cursor.png";
    pub const SELECT_CURSOR_ANIMATION: &str = "resources/scenes/shared/select_cursor.animation";
    pub const SCROLLBAR_THUMB: &str = "resources/scenes/shared/scrollbar.png";
    pub const CONTEXT_MENU: &str = "resources/scenes/shared/context_menu.png";
    pub const CONTEXT_MENU_ANIMATION: &str = "resources/scenes/shared/context_menu.animation";
    pub const ELEMENTS: &str = "resources/scenes/shared/elements.png";
    pub const CARD_ICON_MISSING: &str = "resources/scenes/shared/card_icon_missing.png";
    pub const CARD_PREVIEW_MISSING: &str = "resources/scenes/shared/card_missing.png";
    pub const FULL_CARD: &str = "resources/scenes/shared/full_card.png";
    pub const FULL_CARD_ANIMATION: &str = "resources/scenes/shared/full_card.animation";
    pub const HEALTH_FRAME: &str = "resources/scenes/shared/health_frame.png";
    pub const HEALTH_FRAME_ANIMATION: &str = "resources/scenes/shared/health_frame.animation";
    pub const UNREAD: &str = "resources/scenes/shared/unread.png";
    pub const UNREAD_ANIMATION: &str = "resources/scenes/shared/unread.animation";

    // Textbox
    pub const TEXTBOX_CURSOR: &str = "resources/scenes/shared/textbox_cursor.png";
    pub const TEXTBOX_CURSOR_ANIMATION: &str = "resources/scenes/shared/textbox_cursor.animation";
    pub const TEXTBOX_NEXT: &str = "resources/scenes/shared/textbox_next.png";
    pub const TEXTBOX_NEXT_ANIMATION: &str = "resources/scenes/shared/textbox_next.animation";
    pub const NAVIGATION_TEXTBOX: &str = "resources/scenes/shared/navigation_textbox.png";
    pub const NAVIGATION_TEXTBOX_ANIMATION: &str =
        "resources/scenes/shared/navigation_textbox.animation";

    // BootScene
    pub const BOOT_LOGO: &str = "resources/scenes/boot/logo.png";

    // MainMenuScene
    pub const MAIN_MENU_BG: &str = "resources/scenes/main_menu/bg.png";
    pub const MAIN_MENU_PARTS: &str = "resources/scenes/main_menu/menu.png";
    pub const MAIN_MENU_PARTS_ANIMATION: &str = "resources/scenes/main_menu/menu.animation";
    pub const MAIN_MENU_FOLDER: &str = "resources/scenes/main_menu/folders.png";
    pub const MAIN_MENU_FOLDER_ANIMATION: &str = "resources/scenes/main_menu/folders.animation";
    pub const MAIN_MENU_WINDOWS: &str = "resources/scenes/main_menu/windows.png";
    pub const MAIN_MENU_WINDOWS_ANIMATION: &str = "resources/scenes/main_menu/windows.animation";

    // ServerListScene
    pub const SERVER_LIST_SHEET: &str = "resources/scenes/server_list/sheet.png";
    pub const SERVER_LIST_SHEET_ANIMATION: &str = "resources/scenes/server_list/sheet.animation";

    // InitialConnectScene
    pub const INITIAL_CONNECT_BG: &str = "resources/scenes/initial_connection/bg.png";
    pub const INITIAL_CONNECT_BG_ANIMATION: &str =
        "resources/scenes/initial_connection/bg.animation";

    // OverworldSceneBase
    pub const OVERWORLD_TEXTBOX: &str = "resources/scenes/overworld/textbox.png";
    pub const OVERWORLD_TEXTBOX_ANIMATION: &str = "resources/scenes/overworld/textbox.animation";
    pub const OVERWORLD_WARP: &str = "resources/scenes/overworld/warp.png";
    pub const OVERWORLD_WARP_ANIMATION: &str = "resources/scenes/overworld/warp.animation";
    pub const OVERWORLD_BBS: &str = "resources/scenes/overworld/bbs/bbs.png";
    pub const OVERWORLD_BBS_ANIMATION: &str = "resources/scenes/overworld/bbs/bbs.animation";

    // FoldersScene
    pub const FOLDERS_BG: &str = "resources/scenes/folder_list/bg.png";
    pub const FOLDERS_BG_ANIMATION: &str = "resources/scenes/folder_list/bg.animation";
    pub const FOLDERS_ENABLED: &str = "resources/scenes/folder_list/enabled.png";
    pub const FOLDERS_DISABLED: &str = "resources/scenes/folder_list/disabled.png";
    pub const FOLDERS_CURSOR: &str = "resources/scenes/folder_list/cursor.png";
    pub const FOLDERS_CURSOR_ANIMATION: &str = "resources/scenes/folder_list/cursor.animation";
    pub const FOLDERS_EQUIPPED: &str = "resources/scenes/folder_list/equip.png";
    pub const FOLDERS_EQUIPPED_ANIMATION: &str = "resources/scenes/folder_list/equip.animation";

    // FolderScene
    pub const FOLDER_BG: &str = "resources/scenes/folder_edit/bg.png";
    pub const FOLDER_SIZE: &str = "resources/scenes/folder_edit/size.png";
    pub const FOLDER_DOCK: &str = "resources/scenes/folder_edit/dock.png";
    pub const FOLDER_DOCK_ANIMATION: &str = "resources/scenes/folder_edit/dock.animation";
    pub const FOLDER_PACK_DOCK: &str = "resources/scenes/folder_edit/pack_dock.png";
    pub const FOLDER_PACK_DOCK_ANIMATION: &str = "resources/scenes/folder_edit/pack_dock.animation";
    pub const FOLDER_SORT_MENU: &str = "resources/scenes/folder_edit/sort.png";

    // LibraryScene
    pub const LIBRARY_BG: &str = "resources/scenes/library/bg.png";
    pub const LIBRARY_BG_ANIMATION: &str = "resources/scenes/library/bg.animation";
    pub const LIBRARY_DOCK: &str = "resources/scenes/library/dock.png";
    pub const LIBRARY_DOCK_ANIMATION: &str = "resources/scenes/library/dock.animation";

    // BattleSelectScene
    pub const BATTLE_SELECT_BG: &str = "resources/scenes/battle_select/bg.png";

    // BattleScene
    pub const BATTLE_BG: &str = "resources/scenes/battle/bg.png";
    pub const BATTLE_BG_ANIMATION: &str = "resources/scenes/battle/bg.animation";
    pub const BATTLE_RED_TILES: &str = "resources/scenes/battle/tile_atlas_red.png";
    pub const BATTLE_BLUE_TILES: &str = "resources/scenes/battle/tile_atlas_blue.png";
    pub const BATTLE_OTHER_TILES: &str = "resources/scenes/battle/tile_atlas_other.png";
    pub const BATTLE_TILE_ANIMATION: &str = "resources/scenes/battle/tiles.animation";
    pub const BATTLE_SHADOW_SMALL: &str = "resources/scenes/battle/shadow_small.png";
    pub const BATTLE_SHADOW_BIG: &str = "resources/scenes/battle/shadow_big.png";
    pub const BATTLE_CHARGE: &str = "resources/scenes/battle/charge.png";
    pub const BATTLE_CHARGE_ANIMATION: &str = "resources/scenes/battle/charge.animation";
    pub const BATTLE_CARD_CHARGE: &str = "resources/scenes/battle/card_charge.png";
    pub const BATTLE_CARD_CHARGE_ANIMATION: &str = "resources/scenes/battle/card_charge.animation";
    pub const BATTLE_TRANSFORM_SHINE: &str = "resources/scenes/battle/transform_shine.png";
    pub const BATTLE_TRANSFORM_SHINE_ANIMATION: &str =
        "resources/scenes/battle/transform_shine.animation";
    pub const BATTLE_CARD_SELECT: &str = "resources/scenes/battle/card_select.png";
    pub const BATTLE_CARD_SELECT_ANIMATION: &str = "resources/scenes/battle/card_select.animation";
    pub const BATTLE_TURN_GAUGE: &str = "resources/scenes/battle/turn_gauge.png";
    pub const BATTLE_TURN_GAUGE_ANIMATION: &str = "resources/scenes/battle/turn_gauge.animation";
    pub const BATTLE_EXPLOSION: &str = "resources/scenes/battle/explosion.png";
    pub const BATTLE_EXPLOSION_ANIMATION: &str = "resources/scenes/battle/explosion.animation";
    pub const BATTLE_SPLASH: &str = "resources/scenes/battle/splash.png";
    pub const BATTLE_SPLASH_ANIMATION: &str = "resources/scenes/battle/splash.animation";
    pub const BATTLE_STATUSES: &str = "resources/scenes/battle/statuses.png";
    pub const BATTLE_STATUSES_ANIMATION: &str = "resources/scenes/battle/statuses.animation";

    pub fn game_folder() -> &'static str {
        &GAME_PATH
    }

    pub fn cache_folder() -> &'static str {
        &GAME_PATH
    }

    pub fn is_absolute(path_str: &str) -> bool {
        use std::path::Path;

        path_str.starts_with("/") || Path::new(&path_str).is_absolute()
    }

    pub fn absolute(path_str: &str) -> String {
        GAME_PATH.clone() + &Self::clean(path_str)
    }

    pub fn clean(path_str: &str) -> String {
        path_clean::clean(&path_str.replace('\\', Self::SEPARATOR))
    }

    pub fn clean_folder(path_str: &str) -> String {
        Self::clean(path_str) + Self::SEPARATOR
    }

    pub fn parent(path_str: &str) -> Option<&str> {
        let index = path_str.rfind('/')?;

        Some(&path_str[..index + 1])
    }

    pub fn shorten(path_str: &str) -> String {
        if path_str.starts_with(&*GAME_PATH) {
            path_str[GAME_PATH.len()..].to_string()
        } else {
            path_str.to_string()
        }
    }
}
