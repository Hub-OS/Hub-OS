use crate::render::PostProcessColorBlindness;
use crate::resources::{
    AssetManager, DEFAULT_INPUT_DELAY, DEFAULT_PACKAGE_REPO, Input, MAX_VOLUME, ResourcePaths,
};
use framework::cfg_macros::{cfg_android, cfg_desktop_and_web};
use framework::input::{Button, Key};
use framework::math::Vec2;
use itertools::Itertools;
use std::collections::HashMap;

#[derive(Default, Clone, Copy, PartialEq, Eq)]
pub enum KeyStyle {
    #[default]
    Mix,
    Wasd,
    Emulator,
}

#[derive(Default, Clone, Copy, PartialEq, Eq)]
pub enum InternalResolution {
    #[default]
    Default,
    Gba,
    Auto,
}

#[derive(Default, Clone, Copy, PartialEq, Eq)]
pub enum DisplayInput {
    #[default]
    Off,
    Simplified,
    Battle,
    Full,
}

#[derive(Clone, PartialEq)]
pub struct Config {
    pub language: Option<String>,
    pub fullscreen: bool,
    pub vsync: bool,
    pub internal_resolution: InternalResolution,
    pub lock_aspect_ratio: bool,
    pub integer_scaling: bool,
    pub snap_resize: bool,
    pub flash_brightness: u8,
    pub brightness: u8,
    pub saturation: u8,
    pub ghosting: u8,
    pub hit_numbers: bool,
    pub heal_numbers: bool,
    pub drain_numbers: bool,
    pub display_input: DisplayInput,
    pub color_blindness: u8,
    pub music: u8,
    pub sfx: u8,
    pub mute_music: bool,
    pub mute_sfx: bool,
    pub audio_device: String,
    pub key_style: KeyStyle,
    pub key_bindings: HashMap<Input, Vec<Key>>,
    pub controller_bindings: HashMap<Input, Vec<Button>>,
    pub controller_index: usize,
    pub virtual_input_positions: HashMap<Button, Vec2>,
    pub virtual_controller_scale: f32,
    pub input_delay: u8,
    pub force_relay: bool,
    pub package_repo: String,
}

impl Config {
    pub fn validate(&self) -> bool {
        self.validate_keys() && self.validate_controller()
    }

    pub fn validate_keys(&self) -> bool {
        Config::validate_bindings(&self.key_bindings)
    }

    pub fn validate_controller(&self) -> bool {
        Config::validate_bindings(&self.controller_bindings)
    }

    pub fn with_default_key_bindings(mut self, key_style: KeyStyle) -> Self {
        self.key_bindings = Self::default_key_bindings(key_style);
        self
    }

    pub fn default_key_bindings(key_style: KeyStyle) -> HashMap<Input, Vec<Key>> {
        match key_style {
            KeyStyle::Mix => HashMap::from([
                (Input::Up, vec![Key::W, Key::Up]),
                (Input::Down, vec![Key::S, Key::Down]),
                (Input::Left, vec![Key::A, Key::Left]),
                (Input::Right, vec![Key::D, Key::Right]),
                (Input::ShoulderL, vec![Key::Q, Key::C]),
                (Input::ShoulderR, vec![Key::E, Key::V]),
                (Input::Flee, vec![Key::Q, Key::C]),
                (Input::Info, vec![Key::E, Key::V]),
                (Input::EndTurn, vec![Key::Q, Key::E, Key::C, Key::V]),
                (Input::FaceLeft, vec![Key::R, Key::Backspace]),
                (Input::FaceRight, vec![Key::R, Key::Backspace]),
                (Input::Confirm, vec![Key::Space, Key::J, Key::Z]),
                (Input::UseCard, vec![Key::Space, Key::J, Key::Z]),
                (
                    Input::Cancel,
                    vec![Key::LShift, Key::K, Key::Escape, Key::X],
                ),
                (Input::Shoot, vec![Key::LShift, Key::K, Key::X]),
                (Input::Sprint, vec![Key::LShift, Key::K, Key::X]),
                (Input::Map, vec![Key::M]),
                (Input::Option, vec![Key::Return]),
                (Input::Option2, vec![Key::R, Key::Backspace]),
                (Input::Special, vec![Key::F, Key::L]),
                (Input::End, vec![Key::Return]),
                (Input::Pause, vec![Key::Escape, Key::Return]),
                (Input::RewindFrame, vec![Key::Comma]),
                (Input::AdvanceFrame, vec![Key::Period]),
            ]),
            KeyStyle::Wasd => HashMap::from([
                (Input::Up, vec![Key::W]),
                (Input::Down, vec![Key::S]),
                (Input::Left, vec![Key::A]),
                (Input::Right, vec![Key::D]),
                (Input::ShoulderL, vec![Key::Q]),
                (Input::ShoulderR, vec![Key::E]),
                (Input::Flee, vec![Key::Q]),
                (Input::Info, vec![Key::E]),
                (Input::EndTurn, vec![Key::Q, Key::E]),
                (Input::FaceLeft, vec![Key::R]),
                (Input::FaceRight, vec![Key::R]),
                (Input::Confirm, vec![Key::Space, Key::J]),
                (Input::UseCard, vec![Key::Space, Key::J]),
                (Input::Cancel, vec![Key::LShift, Key::K, Key::Escape]),
                (Input::Shoot, vec![Key::LShift, Key::K]),
                (Input::Sprint, vec![Key::LShift, Key::K]),
                (Input::Map, vec![Key::M]),
                (Input::Option, vec![Key::Return]),
                (Input::Option2, vec![Key::R]),
                (Input::Special, vec![Key::F, Key::L]),
                (Input::End, vec![Key::Return]),
                (Input::Pause, vec![Key::Escape, Key::Return]),
                (Input::RewindFrame, vec![Key::Left]),
                (Input::AdvanceFrame, vec![Key::Right]),
            ]),
            KeyStyle::Emulator => HashMap::from([
                (Input::Up, vec![Key::Up]),
                (Input::Down, vec![Key::Down]),
                (Input::Left, vec![Key::Left]),
                (Input::Right, vec![Key::Right]),
                (Input::ShoulderL, vec![Key::A]),
                (Input::ShoulderR, vec![Key::S]),
                (Input::Flee, vec![Key::A]),
                (Input::Info, vec![Key::S]),
                (Input::EndTurn, vec![Key::A, Key::S]),
                (Input::FaceLeft, vec![Key::LShift]),
                (Input::FaceRight, vec![Key::LShift]),
                (Input::Confirm, vec![Key::Z]),
                (Input::UseCard, vec![Key::Z]),
                (Input::Cancel, vec![Key::X]),
                (Input::Shoot, vec![Key::X]),
                (Input::Sprint, vec![Key::X]),
                (Input::Map, vec![Key::M]),
                (Input::Option, vec![Key::Return]),
                (Input::Option2, vec![Key::Backspace]),
                (Input::Special, vec![Key::C]),
                (Input::End, vec![Key::Return]),
                (Input::Pause, vec![Key::Return]),
                (Input::RewindFrame, vec![Key::Q]),
                (Input::AdvanceFrame, vec![Key::W]),
            ]),
        }
    }

    pub fn default_controller_bindings() -> HashMap<Input, Vec<Button>> {
        HashMap::from([
            (Input::Up, vec![Button::LeftStickUp, Button::DPadUp]),
            (Input::Down, vec![Button::LeftStickDown, Button::DPadDown]),
            (Input::Left, vec![Button::LeftStickLeft, Button::DPadLeft]),
            (
                Input::Right,
                vec![Button::LeftStickRight, Button::DPadRight],
            ),
            (
                Input::ShoulderL,
                vec![Button::LeftTrigger, Button::LeftShoulder],
            ),
            (
                Input::ShoulderR,
                vec![Button::RightTrigger, Button::RightShoulder],
            ),
            (Input::Flee, vec![Button::LeftTrigger, Button::LeftShoulder]),
            (
                Input::Info,
                vec![Button::RightTrigger, Button::RightShoulder],
            ),
            (
                Input::EndTurn,
                vec![
                    Button::LeftTrigger,
                    Button::RightTrigger,
                    Button::LeftShoulder,
                    Button::RightShoulder,
                ],
            ),
            (Input::Confirm, vec![Button::A]),
            (Input::UseCard, vec![Button::A]),
            (Input::Cancel, vec![Button::B]),
            (Input::Shoot, vec![Button::B]),
            (Input::Sprint, vec![Button::B]),
            (Input::Special, vec![Button::X]),
            (Input::Map, vec![Button::Y]),
            (Input::FaceLeft, vec![Button::Y, Button::Select]),
            (Input::FaceRight, vec![Button::Y, Button::Select]),
            (Input::Option, vec![Button::Start]),
            (Input::Option2, vec![Button::Select]),
            (Input::End, vec![Button::Start]),
            (Input::Pause, vec![Button::Start]),
            (Input::RewindFrame, vec![]),
            (Input::AdvanceFrame, vec![]),
        ])
    }

    pub fn default_virtual_input_positions() -> HashMap<Button, Vec2> {
        HashMap::from([
            // left inputs
            (Button::Select, Vec2::new(27.0, -9.0)),
            (Button::LeftTrigger, Vec2::new(0.0, -97.0)),
            (Button::LeftStick, Vec2::new(44.0, -51.0)),
            // right inputs
            (Button::Start, Vec2::new(-27.0, -9.0)),
            (Button::RightTrigger, Vec2::new(0.0, -97.0)),
            (Button::A, Vec2::new(-44.0, -33.0)),
            (Button::B, Vec2::new(-25.0, -52.0)),
            (Button::X, Vec2::new(-63.0, -52.0)),
            (Button::Y, Vec2::new(-44.0, -71.0)),
        ])
    }

    pub fn music_volume(&self) -> f32 {
        if self.mute_music {
            return 0.0;
        }

        self.music as f32 / MAX_VOLUME as f32
    }

    pub fn sfx_volume(&self) -> f32 {
        if self.mute_sfx {
            return 0.0;
        }

        self.sfx as f32 / MAX_VOLUME as f32
    }

    pub fn load(assets: &impl AssetManager) -> Self {
        let path = ResourcePaths::data_folder_absolute("config.ini");
        let config_text = assets.text_silent(&path);

        if config_text.is_empty() {
            let default_config = Config::default();
            default_config.save();
            return default_config;
        }

        let mut config = Config::from(config_text.as_str());

        if !config.validate_keys() {
            log::error!("Invalid key bindings, using defaults");
            config.key_bindings = Self::default_key_bindings(config.key_style);
        }

        if !config.validate_controller() {
            log::error!("Invalid controller bindings, using defaults");
            config.controller_bindings = Self::default_controller_bindings();
        }

        config
    }

    pub fn save(&self) {
        let path = ResourcePaths::data_folder_absolute("config.ini");
        if let Err(err) = std::fs::write(path, self.to_string()) {
            log::error!("Failed to save config: {err:?}");
        }
    }

    fn validate_bindings<V: std::cmp::PartialEq>(bindings: &HashMap<Input, Vec<V>>) -> bool {
        let required_are_set = Input::REQUIRED
            .iter()
            .map(|input| bindings.get(input))
            .all(|binding| binding.is_some_and(|b| !b.is_empty()));

        let non_overlap_bindings: Vec<_> = Input::NON_OVERLAP
            .iter()
            .map(|input| bindings.get(input))
            .collect();

        let non_overlap_are_unique = non_overlap_bindings.iter().all(|binding| {
            non_overlap_bindings
                .iter()
                .filter(|other_input| binding == *other_input)
                .count()
                == 1
        });

        // all required bindings must be set
        required_are_set && non_overlap_are_unique
    }
}

impl Default for Config {
    fn default() -> Config {
        Config {
            language: None,
            fullscreen: {
                cfg_android! {true}
                cfg_desktop_and_web! {false}
            },
            vsync: true,
            internal_resolution: Default::default(),
            lock_aspect_ratio: true,
            integer_scaling: false,
            snap_resize: false,
            flash_brightness: 100,
            brightness: 100,
            saturation: 100,
            ghosting: 0,
            hit_numbers: false,
            heal_numbers: false,
            drain_numbers: false,
            display_input: Default::default(),
            color_blindness: PostProcessColorBlindness::TOTAL_OPTIONS,
            music: MAX_VOLUME / 2,
            sfx: MAX_VOLUME / 2,
            mute_music: false,
            mute_sfx: false,
            audio_device: String::new(),
            key_style: Default::default(),
            key_bindings: Self::default_key_bindings(Default::default()),
            controller_bindings: Self::default_controller_bindings(),
            controller_index: 0,
            virtual_input_positions: Self::default_virtual_input_positions(),
            virtual_controller_scale: 1.0,
            input_delay: DEFAULT_INPUT_DELAY,
            force_relay: false,
            package_repo: String::from(DEFAULT_PACKAGE_REPO),
        }
    }
}

impl From<&str> for Config {
    fn from(s: &str) -> Self {
        use std::str::FromStr;
        use structures::parse_util::*;
        use strum::IntoEnumIterator;

        let mut config = Config {
            language: None,
            fullscreen: false,
            vsync: true,
            internal_resolution: Default::default(),
            lock_aspect_ratio: true,
            integer_scaling: false,
            snap_resize: false,
            flash_brightness: 100,
            brightness: 100,
            saturation: 100,
            ghosting: 0,
            hit_numbers: false,
            heal_numbers: false,
            drain_numbers: false,
            display_input: Default::default(),
            color_blindness: PostProcessColorBlindness::TOTAL_OPTIONS,
            music: MAX_VOLUME,
            sfx: MAX_VOLUME,
            mute_music: false,
            mute_sfx: false,
            audio_device: String::new(),
            key_style: Default::default(),
            key_bindings: HashMap::new(),
            controller_bindings: HashMap::new(),
            controller_index: 0,
            virtual_input_positions: Self::default_virtual_input_positions(),
            virtual_controller_scale: 1.0,
            input_delay: DEFAULT_INPUT_DELAY,
            force_relay: false,
            package_repo: String::from(DEFAULT_PACKAGE_REPO),
        };

        use ini::Ini;

        let ini = match Ini::load_from_str(s) {
            Ok(ini) => ini,
            Err(e) => {
                log::error!("Failed to parse config.ini file: {e}");
                return config;
            }
        };

        if let Some(properties) = ini.section(Some("Video")) {
            config.language = properties.get("Language").map(String::from);
            config.fullscreen = parse_or_default(properties.get("Fullscreen"));
            config.vsync = parse_or(properties.get("VSync"), true);
            config.internal_resolution = match properties
                .get("InternalResolution")
                .unwrap_or_default()
                .to_lowercase()
                .as_str()
            {
                "gba" => InternalResolution::Gba,
                "auto" => InternalResolution::Auto,
                _ => InternalResolution::Default,
            };
            config.lock_aspect_ratio = parse_or_default(properties.get("LockAspectRatio"));
            config.integer_scaling = parse_or_default(properties.get("IntegerScaling"));
            config.snap_resize = parse_or_default(properties.get("SnapResize"));
            config.flash_brightness = parse_or(properties.get("FlashBrightness"), 100);
            config.brightness = parse_or(properties.get("Brightness"), 100);
            config.saturation = parse_or(properties.get("Saturation"), 100);
            config.ghosting = parse_or_default(properties.get("Ghosting"));
            config.hit_numbers = parse_or_default(properties.get("HitParticles"));
            config.heal_numbers = parse_or_default(properties.get("HealParticles"));
            config.drain_numbers = parse_or_default(properties.get("DrainParticles"));
            config.color_blindness = parse_or(
                properties.get("ColorBlindness"),
                PostProcessColorBlindness::TOTAL_OPTIONS,
            );
            config.display_input = match properties
                .get("DisplayInput")
                .unwrap_or_default()
                .to_lowercase()
                .as_str()
            {
                "simplified" => DisplayInput::Simplified,
                "battle" => DisplayInput::Battle,
                "full" => DisplayInput::Full,
                _ => DisplayInput::Off,
            };
        }

        if let Some(properties) = ini.section(Some("Audio")) {
            config.music = parse_or_default::<u8>(properties.get("Music")).min(MAX_VOLUME);
            config.sfx = parse_or_default::<u8>(properties.get("SFX")).min(MAX_VOLUME);
            config.mute_music = parse_or_default(properties.get("MuteMusic"));
            config.mute_sfx = parse_or_default(properties.get("MuteSFX"));
            config.audio_device = properties
                .get("OutputDevice")
                .unwrap_or_default()
                .to_string();
        }

        if let Some(properties) = ini.section(Some("Keyboard")) {
            let key_style_str = properties.get("Style").unwrap_or_default();

            config.key_style = match key_style_str.to_lowercase().as_str() {
                "mix" => KeyStyle::Mix,
                "wasd" => KeyStyle::Wasd,
                "emulator" => KeyStyle::Emulator,
                _ => KeyStyle::default(),
            };

            for input in Input::iter() {
                let input_string = format!("{input:?}");

                let keys = properties
                    .get(&input_string)
                    .into_iter()
                    .flat_map(|key_str| key_str.split(','))
                    .flat_map(|value| Key::from_str(value.trim()).ok())
                    .collect();

                config.key_bindings.insert(input, keys);
            }
        }

        if let Some(properties) = ini.section(Some("Controller")) {
            config.controller_index = parse_or_default(properties.get("ControllerIndex"));

            for input in Input::iter() {
                let input_string = format!("{input:?}");

                let buttons = properties
                    .get(&input_string)
                    .into_iter()
                    .flat_map(|key_str| key_str.split(','))
                    .flat_map(|value| Button::from_str(value.trim()).ok())
                    .collect();

                config.controller_bindings.insert(input, buttons);
            }
        }

        if let Some(properties) = ini.section(Some("VirtualController")) {
            config.virtual_controller_scale = parse_or_default(properties.get("Scale"));

            for (button, position) in &mut config.virtual_input_positions {
                let button_str: &'static str = button.into();

                let Some(position_str) = properties.get(button_str) else {
                    continue;
                };

                let Some((x_str, y_str)) = position_str.split_once(',') else {
                    continue;
                };

                let (Ok(x), Ok(y)) = (x_str.trim().parse::<f32>(), y_str.trim().parse::<f32>())
                else {
                    continue;
                };

                position.x = x;
                position.y = y;
            }
        }

        if let Some(properties) = ini.section(Some("Online")) {
            config.input_delay = parse_or(properties.get("InputDelay"), DEFAULT_INPUT_DELAY);
            config.force_relay = parse_or(properties.get("ForceRelay"), false);

            config.package_repo = properties
                .get("PackageRepo")
                .unwrap_or(DEFAULT_PACKAGE_REPO)
                .to_string();

            if config.package_repo.is_empty() {
                config.package_repo = String::from(DEFAULT_PACKAGE_REPO);
            }
        }

        config
    }
}

impl std::fmt::Display for Config {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        use strum::IntoEnumIterator;

        writeln!(f, "[Video]")?;

        if let Some(language) = &self.language {
            writeln!(f, "Language = {language}")?;
        }

        writeln!(f, "Fullscreen = {}", self.fullscreen)?;
        writeln!(f, "VSync = {}", self.vsync)?;

        match self.internal_resolution {
            InternalResolution::Default => writeln!(f, "InternalResolution = default")?,
            InternalResolution::Gba => writeln!(f, "InternalResolution = gba")?,
            InternalResolution::Auto => writeln!(f, "InternalResolution = auto")?,
        }

        writeln!(f, "LockAspectRatio = {}", self.lock_aspect_ratio)?;
        writeln!(f, "IntegerScaling = {}", self.integer_scaling)?;
        writeln!(f, "SnapResize = {}", self.snap_resize)?;
        writeln!(f, "FlashBrightness = {}", self.flash_brightness)?;
        writeln!(f, "Brightness = {}", self.brightness)?;
        writeln!(f, "Saturation = {}", self.saturation)?;
        writeln!(f, "Ghosting = {}", self.ghosting)?;
        writeln!(f, "HitParticles = {}", self.hit_numbers)?;
        writeln!(f, "HealParticles = {}", self.heal_numbers)?;
        writeln!(f, "DrainParticles = {}", self.drain_numbers)?;
        writeln!(f, "ColorBlindness = {}", self.color_blindness)?;

        match self.display_input {
            DisplayInput::Off => writeln!(f, "DisplayInput = off")?,
            DisplayInput::Simplified => writeln!(f, "DisplayInput = simplified")?,
            DisplayInput::Battle => writeln!(f, "DisplayInput = battle")?,
            DisplayInput::Full => writeln!(f, "DisplayInput = full")?,
        }

        writeln!(f, "[Audio]")?;
        writeln!(f, "Music = {}", self.music)?;
        writeln!(f, "SFX = {}", self.sfx)?;
        writeln!(f, "MuteMusic = {}", self.mute_music)?;
        writeln!(f, "MuteSFX = {}", self.mute_sfx)?;
        writeln!(f, "OutputDevice = {}", self.audio_device)?;

        writeln!(f, "[Keyboard]")?;

        match self.key_style {
            KeyStyle::Mix => writeln!(f, "Style = Mix")?,
            KeyStyle::Wasd => writeln!(f, "Style = WASD")?,
            KeyStyle::Emulator => writeln!(f, "Style = Emulator")?,
        }

        for input in Input::iter() {
            write!(f, "{input:?} = ")?;

            if let Some(keys) = self.key_bindings.get(&input) {
                let keys_string = keys
                    .iter()
                    .map(|key| -> &'static str { key.into() })
                    .join(",");

                writeln!(f, "{keys_string}")?;
            } else {
                writeln!(f, "None")?;
            }
        }

        writeln!(f, "[Controller]")?;
        writeln!(f, "ControllerIndex = {}", self.controller_index)?;

        for input in Input::iter() {
            write!(f, "{input:?} = ")?;

            if let Some(buttons) = self.controller_bindings.get(&input) {
                let buttons_string = buttons
                    .iter()
                    .map(|button| -> &'static str { button.into() })
                    .join(",");

                writeln!(f, "{buttons_string}")?;
            } else {
                writeln!(f, "None")?;
            }
        }

        writeln!(f, "[VirtualController]")?;
        writeln!(f, "Scale = {}", self.virtual_controller_scale)?;

        for (button, position) in &self.virtual_input_positions {
            let button_str: &'static str = button.into();
            writeln!(f, "{button_str} = {},{}", position.x, position.y)?;
        }

        writeln!(f, "[Online]")?;
        writeln!(f, "InputDelay = {}", self.input_delay)?;
        writeln!(f, "ForceRelay = {}", self.force_relay)?;

        if self.package_repo != DEFAULT_PACKAGE_REPO {
            writeln!(f, "PackageRepo = {}", self.package_repo)?;
        } else {
            writeln!(f, "PackageRepo = ",)?;
        }

        Ok(())
    }
}
