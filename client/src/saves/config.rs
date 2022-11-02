use crate::resources::{AssetManager, Input};
use framework::input::{Button, Key};
use std::collections::HashMap;

pub struct Config {
    pub fullscreen: bool,
    pub music: u8,
    pub sfx: u8,
    pub key_bindings: HashMap<Input, Key>,
    pub controller_bindings: HashMap<Input, Button>,
    pub controller_index: usize,
}

impl Config {
    pub fn validate(&self) -> bool {
        Config::validate_bindings(&self.key_bindings)
    }

    pub fn load(assets: &impl AssetManager) -> Self {
        let config_text = assets.text("config.ini");

        if config_text.is_empty() {
            let default_config = Config::default();
            let _ = std::fs::write("config.ini", default_config.to_string());
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

    fn validate_bindings<K: std::cmp::PartialEq>(bindings: &HashMap<Input, K>) -> bool {
        const REQUIRED_INPUTS: [Input; 10] = [
            Input::Up,
            Input::Down,
            Input::Left,
            Input::Right,
            Input::Cancel,
            Input::Confirm,
            Input::Pause,
            Input::Option,
            Input::ShoulderL,
            Input::ShoulderR,
        ];

        let required_bindings = REQUIRED_INPUTS
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
            (Input::Option, Key::F),
            (Input::ShoulderL, Key::Q),
            (Input::ShoulderR, Key::E),
            (Input::Confirm, Key::Space),
            (Input::UseCard, Key::Space),
            (Input::Cancel, Key::LShift),
            (Input::Shoot, Key::LShift),
            (Input::Sprint, Key::LShift),
            (Input::Minimap, Key::M),
            (Input::Pause, Key::Escape),
        ]);

        let controller_bindings = HashMap::new();

        Config {
            fullscreen: false,
            music: 3,
            sfx: 3,
            key_bindings,
            controller_bindings,
            controller_index: 0,
        }
    }
}

impl From<&str> for Config {
    fn from(s: &str) -> Self {
        use crate::parse_util::parse_or_default;
        use std::str::FromStr;
        use strum::IntoEnumIterator;

        let mut config = Config {
            fullscreen: false,
            music: 3,
            sfx: 3,
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
        }

        if let Some(properties) = ini.section(Some("Audio")) {
            config.music = parse_or_default::<u8>(properties.get("Music")).min(3);
            config.sfx = parse_or_default::<u8>(properties.get("SFX")).min(3);
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

            writeln!(s, "[Audio]")?;
            writeln!(s, "Music = {}", self.music)?;
            writeln!(s, "SFX = {}", self.sfx)?;

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
