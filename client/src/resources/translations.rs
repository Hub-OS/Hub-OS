use crate::resources::ResourcePaths;
use crate::saves::Config;
use fluent_templates::Loader;
use framework::input::Button;
use std::collections::HashMap;

pub use fluent_templates::fluent_bundle::FluentValue;

pub type TranslationArgs<'key, 'value> = Vec<(&'key str, crate::FluentValue<'value>)>;

pub struct Translations {
    language: fluent_templates::LanguageIdentifier,
    loader: fluent_templates::ArcLoader,
    locales: Vec<fluent_templates::LanguageIdentifier>,
}

impl Translations {
    pub fn new(config: &Config) -> Self {
        let loader = fluent_templates::ArcLoader::builder(
            &format!("{}resources/locales/", ResourcePaths::game_folder()),
            fluent_templates::langid!("en-US"),
        )
        .customize(|bundle| bundle.set_use_isolating(false))
        .build()
        .unwrap();

        // resolve requested locales based on system languages and config
        let mut requested_locales = sys_locale::get_locales()
            .flat_map(|identifer| identifer.parse::<fluent_langneg::LanguageIdentifier>())
            .collect::<Vec<_>>();

        if let Some(language_id) = config
            .language
            .as_ref()
            .and_then(|l| l.parse::<fluent_langneg::LanguageIdentifier>().ok())
        {
            requested_locales.retain(|existing_id| *existing_id != language_id);
            requested_locales.insert(0, language_id);
        }

        // resolve a language from the requested and supported languages
        let supported_languages = loader
            .locales()
            .flat_map(|s| s.to_string().parse::<fluent_langneg::LanguageIdentifier>())
            .collect::<Vec<_>>();

        let language = fluent_langneg::negotiate_languages(
            &requested_locales,
            &supported_languages,
            None,
            fluent_langneg::NegotiationStrategy::Lookup,
        )
        .first()
        .and_then(|language| language.to_string().parse().ok())
        .unwrap_or(loader.fallback().clone());

        let mut locales: Vec<_> = loader.locales().cloned().collect();
        locales.sort();

        Self {
            language,
            loader,
            locales,
        }
    }

    pub fn locales(&self) -> &[fluent_templates::LanguageIdentifier] {
        &self.locales
    }

    pub fn translate(&self, text_key: &str) -> String {
        self.translate_with_args(text_key, vec![])
    }

    pub fn translate_with_args(&self, text_key: &str, args: TranslationArgs) -> String {
        let args = if args.is_empty() {
            None
        } else {
            Some(HashMap::<&str, FluentValue>::from_iter(args))
        };

        match self
            .loader
            .lookup_single_language(&self.language, text_key, args.as_ref())
        {
            Ok(s) => s,
            Err(err) => {
                log::error!("{err}");
                text_key.to_string()
            }
        }
    }

    pub fn button_key(button: Button) -> &'static str {
        match button {
            Button::Meta => "button-meta",
            Button::Start => "button-start",
            Button::Select => "button-select",
            Button::A => "button-a",
            Button::B => "button-b",
            Button::X => "button-x",
            Button::Y => "button-y",
            Button::C => "button-c",
            Button::Z => "button-z",
            Button::LeftShoulder => "button-left-shoulder",
            Button::LeftTrigger => "button-left-trigger",
            Button::RightShoulder => "button-right-shoulder",
            Button::RightTrigger => "button-right-trigger",
            Button::LeftStick => "button-left-stick",
            Button::RightStick => "button-right-stick",
            Button::DPadUp => "button-d-pad-up",
            Button::DPadDown => "button-d-pad-down",
            Button::DPadLeft => "button-d-pad-left",
            Button::DPadRight => "button-d-pad-right",
            Button::LeftStickUp => "button-left-stick-up",
            Button::LeftStickDown => "button-left-stick-down",
            Button::LeftStickLeft => "button-left-stick-left",
            Button::LeftStickRight => "button-left-stick-right",
            Button::RightStickUp => "button-right-stick-up",
            Button::RightStickDown => "button-right-stick-down",
            Button::RightStickLeft => "button-right-stick-left",
            Button::RightStickRight => "button-right-stick-right",
            Button::Paddle1 => "button-paddle1",
            Button::Paddle2 => "button-paddle2",
            Button::Paddle3 => "button-paddle3",
            Button::Paddle4 => "button-paddle4",
        }
    }

    pub fn button_short_key(button: Button) -> &'static str {
        match button {
            Button::Meta => "button-meta-short",
            Button::Start => "button-start-short",
            Button::Select => "button-select-short",
            Button::A => "button-a-short",
            Button::B => "button-b-short",
            Button::X => "button-x-short",
            Button::Y => "button-y-short",
            Button::C => "button-c-short",
            Button::Z => "button-z-short",
            Button::LeftShoulder => "button-left-shoulder-short",
            Button::LeftTrigger => "button-left-trigger-short",
            Button::RightShoulder => "button-right-shoulder-short",
            Button::RightTrigger => "button-right-trigger-short",
            Button::LeftStick => "button-left-stick-short",
            Button::RightStick => "button-right-stick-short",
            Button::DPadUp => "button-d-pad-up-short",
            Button::DPadDown => "button-d-pad-down-short",
            Button::DPadLeft => "button-d-pad-left-short",
            Button::DPadRight => "button-d-pad-right-short",
            Button::LeftStickUp => "button-left-stick-up-short",
            Button::LeftStickDown => "button-left-stick-down-short",
            Button::LeftStickLeft => "button-left-stick-left-short",
            Button::LeftStickRight => "button-left-stick-right-short",
            Button::RightStickUp => "button-right-stick-up-short",
            Button::RightStickDown => "button-right-stick-down-short",
            Button::RightStickLeft => "button-right-stick-left-short",
            Button::RightStickRight => "button-right-stick-right-short",
            Button::Paddle1 => "button-paddle1-short",
            Button::Paddle2 => "button-paddle2-short",
            Button::Paddle3 => "button-paddle3-short",
            Button::Paddle4 => "button-paddle4-short",
        }
    }
}
