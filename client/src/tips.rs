use crate::resources::Globals;
use framework::common::GameIO;
use packets::structures::Input;

pub enum Tip {
    ChangeDisplayName,
    ViewDisplayName,
    UpdatingMods,
    ServerMusic,
    OpenMap,
    OpenEmotes,
    TurnAround,
    FlippingCards,
    SpecialButton,
    AutoEmotes,
    MoveCardsInBulk,
}

impl Tip {
    pub fn log_random(game_io: &GameIO) {
        use rand::seq::IndexedRandom;

        let tips = &[
            Tip::ChangeDisplayName,
            Tip::ViewDisplayName,
            Tip::UpdatingMods,
            Tip::ServerMusic,
            Tip::OpenMap,
            Tip::OpenEmotes,
            Tip::TurnAround,
            Tip::FlippingCards,
            Tip::SpecialButton,
            Tip::AutoEmotes,
            Tip::MoveCardsInBulk,
        ];

        let tip = tips.choose(&mut rand::rng()).unwrap();

        let globals = Globals::from_resources(game_io);
        let message = tip.create_message(game_io).into();
        let prefixed_message = globals.translate_with_args("tip-prefixed", vec![("tip", message)]);
        log::info!("{prefixed_message}");
    }

    pub fn create_message(&self, game_io: &GameIO) -> String {
        let globals = Globals::from_resources(game_io);

        match self {
            Tip::ChangeDisplayName => globals.translate("tip-change-display-name"),
            Tip::ViewDisplayName => globals.translate("tip-view-display-name"),
            Tip::UpdatingMods => globals.translate("tip-updating-mods"),
            Tip::ServerMusic => globals.translate("tip-server-music"),
            Tip::OpenMap => Self::translate_with_input(globals, "tip-open-map", Input::Map),
            Tip::OpenEmotes => {
                Self::translate_with_input(globals, "tip-open-emotes", Input::Option2)
            }
            Tip::TurnAround => {
                Self::translate_with_input(globals, "tip-turn-around", Input::FaceLeft)
            }
            Tip::FlippingCards => {
                Self::translate_with_input(globals, "tip-flipping-cards", Input::Special)
            }
            Tip::SpecialButton => {
                Self::translate_with_input(globals, "tip-special-button", Input::Special)
            }
            Tip::AutoEmotes => globals.translate("tip-auto-emotes"),
            Tip::MoveCardsInBulk => {
                Self::translate_with_input(globals, "tip-move-cards-in-bulk", Input::Confirm)
            }
        }
    }

    fn translate_with_input(globals: &Globals, text_id: &str, input: Input) -> String {
        globals.translate_with_args(
            text_id,
            vec![(
                "binding",
                Self::resolve_input_binding(globals, input).into(),
            )],
        )
    }

    fn resolve_input_binding(globals: &Globals, input: Input) -> String {
        let config = &globals.config;
        let key_bindings = &config.key_bindings;
        let controller_bindings = &config.controller_bindings;

        let key = key_bindings.get(&input).and_then(|keys| keys.first());
        let button = controller_bindings
            .get(&input)
            .and_then(|buttons| buttons.first());

        let input_string = globals.translate(input.translation_key());

        match (key, button) {
            (Some(key), Some(button)) => globals.translate_with_args(
                "tip-binding-with-key-and-button",
                vec![
                    ("input", input_string.into()),
                    ("key", format!("{key:?}").into()),
                    ("button", format!("{button:?}").into()),
                ],
            ),
            (Some(key), None) => globals.translate_with_args(
                "tip-binding-with-key",
                vec![
                    ("input", input_string.into()),
                    ("key", format!("{key:?}").into()),
                ],
            ),
            (None, Some(button)) => globals.translate_with_args(
                "tip-binding-with-button",
                vec![
                    ("input", input_string.into()),
                    ("button", format!("{button:?}").into()),
                ],
            ),
            (None, None) => input_string,
        }
    }
}
