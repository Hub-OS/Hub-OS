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
}

impl Tip {
    pub fn log_random(game_io: &GameIO) {
        use rand::seq::SliceRandom;

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
        ];

        let tip = tips.choose(&mut rand::thread_rng()).unwrap();

        log::info!("Tip: {}", tip.create_message(game_io));
    }

    pub fn create_message(&self, game_io: &GameIO) -> String {
        match self {
            Tip::ChangeDisplayName => {
                String::from("Your display name can be changed in Config -> Profile -> Nickname.")
            }
            Tip::ViewDisplayName => {
                String::from("You can see display names by placing your cursor over another character in the cyberworld.")
            }
            Tip::UpdatingMods => {
                String::from("You should update mods every once in a while through Config -> Mods -> Update Mods.")
            }
            Tip::ServerMusic => {
                String::from("Resource mods can customize visuals, sounds, and music, including default server music.")
            }
            Tip::OpenMap => {
                let mut message = String::from("The map can be opened with ");
                Self::write_input_binding(game_io, &mut message, Input::Map);
                message.push('.');
                message
            }
            Tip::OpenEmotes => {
                let mut message = String::from("Emotes can be accessed by pressing ");
                Self::write_input_binding(game_io, &mut message, Input::Option2);
                message.push('.');
                message
            }
            Tip::TurnAround => {
                let mut message = String::from("Some battles allow you to turn around using ");
                Self::write_input_binding(game_io, &mut message, Input::FaceLeft);
                message.push('.');
                message
            }
            Tip::FlippingCards => {
                let mut message = String::from("Status effects and other properties are listed on the backs of chips. Use ");
                Self::write_input_binding(game_io, &mut message, Input::Special);
                message.push_str(" to see the back of a chip.");
                message
            }
            Tip::SpecialButton => {
                let mut message = String::from("");
                Self::write_input_binding(game_io, &mut message, Input::Special);
                message.push_str(" is used to activate extra equipment such as shields. This is similar to B+Left from other games.");
                message
            }
            Tip::AutoEmotes => {
                String::from("A thinking emote will appear over players messing with settings.")
            }
        }
    }

    fn write_input_binding(game_io: &GameIO, string: &mut String, input: Input) {
        use std::fmt::Write;

        let globals = game_io.resource::<Globals>().unwrap();
        let config = &globals.config;
        let key_bindings = &config.key_bindings;
        let controller_bindings = &config.controller_bindings;

        let key = key_bindings.get(&input).and_then(|keys| keys.first());
        let button = controller_bindings
            .get(&input)
            .and_then(|buttons| buttons.first());

        let _ = write!(string, "{input:?}");

        if key.is_some() || button.is_some() {
            let _ = write!(string, " (");
        }

        if let Some(key) = key {
            let _ = write!(string, "{key:?} on keyboard");
        }

        if key.is_some() && button.is_some() {
            string.push_str(" or ");
        }

        if let Some(button) = button {
            let _ = write!(string, "{button:?} on a controller");
        }

        if key.is_some() || button.is_some() {
            let _ = write!(string, ")");
        }
    }
}
