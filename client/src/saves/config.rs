use crate::render::PostProcessColorBlindness;
use crate::resources::{AssetManager, Input, MAX_VOLUME};
use framework::input::{Button, Key};
use std::collections::HashMap;

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
    pub key_bindings: HashMap<Input, Key>,
    pub controller_bindings: HashMap<Input, Button>,
    pub controller_index: usize,
}

impl Config {
    pub fn validate(&self) -> bool {
        Config::validate_bindings(&self.key_bindings)
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
            log::error!("config file invalid, falling back to default config");
            return Config::default();
        }

        config
    }

    pub fn save(&self) {
        let _ = std::fs::write("config.ini", self.to_string());
    }

    fn validate_bindings<V: std::cmp::PartialEq>(bindings: &HashMap<Input, V>) -> bool {
        let required_bindings = Input::REQUIRED_INPUTS
            .iter()
            .map(|input| bindings.get(input))
            .collect::<Vec<_>>();

        // all required bindings must be set and unique
        required_bindings.iter().all(|input| {
            input.is_some()
                && required_bindings
                    .iter()
                    .filter(|other_input| input == *other_input)
                    .count()
                    == 1
        })
    }
}

impl Default for Config {
    fn default() -> Config {
        let key_bindings = HashMap::from([
            (Input::Left, Key::A),
            (Input::Right, Key::D),
            (Input::Up, Key::W),
            (Input::Down, Key::S),
            (Input::ShoulderL, Key::Q),
            (Input::ShoulderR, Key::E),
            (Input::Confirm, Key::Space),
            (Input::UseCard, Key::Space),
            (Input::Cancel, Key::LShift),
            (Input::Shoot, Key::LShift),
            (Input::Sprint, Key::LShift),
            (Input::Option, Key::F),
            (Input::Special, Key::F),
            (Input::Minimap, Key::M),
            (Input::Pause, Key::Escape),
            (Input::RewindFrame, Key::Left),
            (Input::AdvanceFrame, Key::Right),
        ]);

        let controller_bindings = HashMap::from([
            (Input::Left, Button::DPadLeft),
            (Input::Right, Button::DPadRight),
            (Input::Up, Button::DPadUp),
            (Input::Down, Button::DPadDown),
            (Input::ShoulderL, Button::LeftTrigger),
            (Input::ShoulderR, Button::RightTrigger),
            (Input::Confirm, Button::A),
            (Input::UseCard, Button::A),
            (Input::Cancel, Button::B),
            (Input::Shoot, Button::B),
            (Input::Sprint, Button::B),
            (Input::Option, Button::X),
            (Input::Special, Button::X),
            (Input::Minimap, Button::Y),
            (Input::Pause, Button::Start),
            (Input::RewindFrame, Button::LeftShoulder),
            (Input::AdvanceFrame, Button::RightShoulder),
        ]);

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
            key_bindings,
            controller_bindings,
            controller_index: 0,
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
            key_bindings: HashMap::new(),
            controller_bindings: HashMap::new(),
            controller_index: 0,
        };

        use ini::Ini;

        let ini = match Ini::load_from_str(s) {
            Ok(ini) => ini,
            Err(e) => {
                log::error!("failed to parse config.ini file: {}", e);
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
            config.controller_index = parse_or_default(properties.get("ControllerIndex"));

            for input in Input::iter() {
                let input_string = format!("{:?}", input);

                let key_str = properties.get(&input_string);
                let key = key_str.and_then(|value| Key::from_str(value).ok());

                if let Some(key) = key {
                    config.key_bindings.insert(input, key);
                } else if key_str != Some("None") && key_str.is_some() {
                    log::error!("failed to parse {:?} in config.ini", key_str.unwrap(),)
                }
            }
        }

        if let Some(properties) = ini.section(Some("Controller")) {
            config.controller_index = parse_or_default(properties.get("ControllerIndex"));

            for input in Input::iter() {
                let input_string = format!("{:?}", input);

                let button_str = properties.get(&input_string);
                let button = button_str.and_then(|value| Button::from_str(value).ok());

                if let Some(button) = button {
                    config.controller_bindings.insert(input, button);
                } else if button_str != Some("None") && button_str.is_some() {
                    log::error!("failed to parse {:?} in config.ini", button_str.unwrap(),)
                }
            }
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
            for input in Input::iter() {
                write!(s, "{:?} = ", input)?;

                if let Some(key) = self.key_bindings.get(&input) {
                    writeln!(s, "{:?}", key)?;
                } else {
                    writeln!(s, "None")?;
                }
            }

            writeln!(s, "[Controller]")?;
            writeln!(s, "Controller Index = {}", self.controller_index)?;

            for input in Input::iter() {
                write!(s, "{:?} = ", input)?;

                if let Some(key) = self.controller_bindings.get(&input) {
                    writeln!(s, "{:?}", key)?;
                } else {
                    writeln!(s, "None")?;
                }
            }

            Ok(s)
        };

        attempt().unwrap()
    }
}
