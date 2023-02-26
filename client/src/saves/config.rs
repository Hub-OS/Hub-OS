use crate::render::PostProcessColorBlindness;
use crate::resources::{AssetManager, Input, DEFAULT_PACKAGE_REPO, MAX_VOLUME};
use framework::input::{Button, Key};
use itertools::Itertools;
use std::collections::HashMap;

#[derive(Default, Clone, Copy, PartialEq, Eq)]
pub enum KeyStyle {
    Wasd,
    #[default]
    Emulator,
}

#[derive(Clone, PartialEq, Eq)]
pub struct Config {
    pub fullscreen: bool,
    pub lock_aspect_ratio: bool,
    pub brightness: u8,
    pub saturation: u8,
    pub ghosting: u8,
    pub color_blindness: u8,
    pub music: u8,
    pub sfx: u8,
    pub mute_music: bool,
    pub mute_sfx: bool,
    pub key_style: KeyStyle,
    pub key_bindings: HashMap<Input, Vec<Key>>,
    pub controller_bindings: HashMap<Input, Vec<Button>>,
    pub controller_index: usize,
    pub package_repo: String,
}

impl Config {
    pub fn validate(&self) -> bool {
        Config::validate_bindings(&self.key_bindings)
    }

    pub fn with_default_key_bindings(mut self, key_style: KeyStyle) -> Self {
        self.key_bindings = Self::default_key_bindings(key_style);
        self
    }

    pub fn default_key_bindings(key_style: KeyStyle) -> HashMap<Input, Vec<Key>> {
        match key_style {
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
                (Input::Confirm, vec![Key::Space]),
                (Input::UseCard, vec![Key::Space]),
                (Input::Cancel, vec![Key::LShift, Key::Escape]),
                (Input::Shoot, vec![Key::LShift]),
                (Input::Sprint, vec![Key::LShift]),
                (Input::Minimap, vec![Key::M]),
                (Input::Option, vec![Key::F]),
                (Input::Special, vec![Key::F]),
                (Input::End, vec![Key::F]),
                (Input::Pause, vec![Key::Escape]),
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
                (Input::Minimap, vec![Key::M]),
                (Input::Option, vec![Key::Return]),
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
            (Input::ShoulderL, vec![Button::LeftTrigger]),
            (Input::ShoulderR, vec![Button::RightTrigger]),
            (Input::Flee, vec![Button::LeftTrigger]),
            (Input::Info, vec![Button::RightTrigger]),
            (
                Input::EndTurn,
                vec![Button::LeftTrigger, Button::RightTrigger],
            ),
            (Input::Confirm, vec![Button::A]),
            (Input::UseCard, vec![Button::A]),
            (Input::Cancel, vec![Button::B]),
            (Input::Shoot, vec![Button::B]),
            (Input::Sprint, vec![Button::B]),
            (Input::Special, vec![Button::X]),
            (Input::Minimap, vec![Button::Y]),
            (Input::FaceLeft, vec![Button::Y]),
            (Input::FaceRight, vec![Button::Y]),
            (Input::Option, vec![Button::Start]),
            (Input::End, vec![Button::Start]),
            (Input::Pause, vec![Button::Start]),
            (Input::RewindFrame, vec![Button::LeftShoulder]),
            (Input::AdvanceFrame, vec![Button::RightShoulder]),
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
        let config_text = assets.text("config.ini");

        if config_text.is_empty() {
            let default_config = Config::default();
            default_config.save();
            return default_config;
        }

        let config = Config::from(config_text.as_str());

        if !config.validate() {
            // todo: don't override the entire config file, patch out / correct what we have
            log::error!("Config file invalid, falling back to default config");
            return Config::default();
        }

        config
    }

    pub fn save(&self) {
        let _ = std::fs::write("config.ini", self.to_string());
    }

    fn validate_bindings<V: std::cmp::PartialEq>(bindings: &HashMap<Input, V>) -> bool {
        let required_are_set = Input::REQUIRED_INPUTS
            .iter()
            .map(|input| bindings.get(input))
            .all(|binding| binding.is_some());

        let non_overlap_bindings: Vec<_> = Input::NON_OVERLAP_INPUTS
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
            fullscreen: false,
            lock_aspect_ratio: true,
            brightness: 100,
            saturation: 100,
            ghosting: 0,
            color_blindness: PostProcessColorBlindness::TOTAL_OPTIONS,
            music: MAX_VOLUME,
            sfx: MAX_VOLUME,
            mute_music: false,
            mute_sfx: false,
            key_style: Default::default(),
            key_bindings: HashMap::new(),
            controller_bindings: Self::default_controller_bindings(),
            controller_index: 0,
            package_repo: String::from(DEFAULT_PACKAGE_REPO),
        }
    }
}

impl From<&str> for Config {
    fn from(s: &str) -> Self {
        use crate::parse_util::*;
        use std::str::FromStr;
        use strum::IntoEnumIterator;

        let mut config = Config {
            fullscreen: false,
            lock_aspect_ratio: true,
            brightness: 100,
            saturation: 100,
            ghosting: 0,
            color_blindness: PostProcessColorBlindness::TOTAL_OPTIONS,
            music: MAX_VOLUME,
            sfx: MAX_VOLUME,
            mute_music: false,
            mute_sfx: false,
            key_style: Default::default(),
            key_bindings: HashMap::new(),
            controller_bindings: HashMap::new(),
            controller_index: 0,
            package_repo: String::from(DEFAULT_PACKAGE_REPO),
        };

        use ini::Ini;

        let ini = match Ini::load_from_str(s) {
            Ok(ini) => ini,
            Err(e) => {
                log::error!("Failed to parse config.ini file: {}", e);
                return config;
            }
        };

        if let Some(properties) = ini.section(Some("Video")) {
            config.fullscreen = parse_or_default(properties.get("Fullscreen"));
            config.lock_aspect_ratio = parse_or_default(properties.get("LockAspectRatio"));
            config.brightness = parse_or(properties.get("Brightness"), 100);
            config.saturation = parse_or(properties.get("Saturation"), 100);
            config.ghosting = parse_or_default(properties.get("Ghosting"));
            config.color_blindness = parse_or(
                properties.get("ColorBlindness"),
                PostProcessColorBlindness::TOTAL_OPTIONS,
            );
        }

        if let Some(properties) = ini.section(Some("Audio")) {
            config.music = parse_or_default::<u8>(properties.get("Music")).min(MAX_VOLUME);
            config.sfx = parse_or_default::<u8>(properties.get("SFX")).min(MAX_VOLUME);
            config.mute_music = parse_or_default(properties.get("MuteMusic"));
            config.mute_sfx = parse_or_default(properties.get("MuteSFX"));
        }

        if let Some(properties) = ini.section(Some("Keyboard")) {
            let key_style_str = properties.get("Style").unwrap_or_default();

            config.key_style = match key_style_str.to_lowercase().as_str() {
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
                    .flat_map(|value| Key::from_str(value).ok())
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
                    .flat_map(|value| Button::from_str(value).ok())
                    .collect();

                config.controller_bindings.insert(input, buttons);
            }
        }

        if let Some(properties) = ini.section(Some("Online")) {
            config.package_repo = properties
                .get("PackageRepo")
                .unwrap_or(DEFAULT_PACKAGE_REPO)
                .to_string();
        }

        config
    }
}

impl ToString for Config {
    fn to_string(&self) -> String {
        use std::fmt::Write;
        use strum::IntoEnumIterator;

        let attempt = || -> Result<String, std::fmt::Error> {
            let mut s = String::new();

            writeln!(s, "[Video]")?;
            writeln!(s, "Fullscreen = {}", self.fullscreen)?;
            writeln!(s, "LockAspectRatio = {}", self.lock_aspect_ratio)?;
            writeln!(s, "Brightness = {}", self.brightness)?;
            writeln!(s, "Saturation = {}", self.saturation)?;
            writeln!(s, "Ghosting = {}", self.ghosting)?;
            writeln!(s, "ColorBlindness = {}", self.color_blindness)?;

            writeln!(s, "[Audio]")?;
            writeln!(s, "Music = {}", self.music)?;
            writeln!(s, "SFX = {}", self.sfx)?;
            writeln!(s, "MuteMusic = {}", self.mute_music)?;
            writeln!(s, "MuteSFX = {}", self.mute_sfx)?;

            writeln!(s, "[Keyboard]")?;

            match self.key_style {
                KeyStyle::Wasd => writeln!(s, "KeyStyle = WASD")?,
                KeyStyle::Emulator => writeln!(s, "KeyStyle = Emulator")?,
            }

            for input in Input::iter() {
                write!(s, "{input:?} = ")?;

                if let Some(keys) = self.key_bindings.get(&input) {
                    let keys_string = keys
                        .iter()
                        .map(|key| -> &'static str { key.into() })
                        .join(",");

                    writeln!(s, "{keys_string}")?;
                } else {
                    writeln!(s, "None")?;
                }
            }

            writeln!(s, "[Controller]")?;
            writeln!(s, "ControllerIndex = {}", self.controller_index)?;

            for input in Input::iter() {
                write!(s, "{input:?} = ")?;

                if let Some(buttons) = self.controller_bindings.get(&input) {
                    let buttons_string = buttons
                        .iter()
                        .map(|button| -> &'static str { button.into() })
                        .join(",");

                    writeln!(s, "{buttons_string}")?;
                } else {
                    writeln!(s, "None")?;
                }
            }

            writeln!(s, "[Online]")?;

            if self.package_repo != DEFAULT_PACKAGE_REPO {
                writeln!(s, "PackageRepo = {}", self.package_repo)?;
            }

            Ok(s)
        };

        attempt().unwrap()
    }
}
