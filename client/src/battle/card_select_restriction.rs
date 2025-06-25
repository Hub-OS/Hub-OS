use super::Player;
use crate::saves::Card;
use packets::structures::PackageId;

#[derive(Clone, Copy)]
pub enum CardSelectRestriction<'a> {
    Code(&'a str),
    Package(&'a PackageId),
    Mixed {
        package_id: &'a PackageId,
        code: &'a str,
    },
    Any,
}

impl<'a> CardSelectRestriction<'a> {
    pub fn resolve(player: &'a Player) -> Self {
        let ids_and_codes: Vec<_> = player
            .staged_items
            .resolve_card_ids_and_codes(&player.deck)
            .collect();

        if ids_and_codes.is_empty() {
            return Self::Any;
        }

        let mut first_package_id: Option<&PackageId> = None;
        let mut code_restriction: Option<&str> = None;

        let mut same_package_id = true;
        let mut same_code = true;

        for (package_id, code) in ids_and_codes {
            if let Some(id) = first_package_id {
                same_package_id &= package_id == id;
            } else {
                first_package_id = Some(package_id);
            }

            if let Some(code_restriction) = code_restriction {
                same_code &= code == code_restriction || code == "*";
            } else if code != "*" {
                code_restriction = Some(code);
            }
        }

        if let Some(code) = code_restriction {
            if code == "!" {
                return Self::Code(code);
            } else if same_package_id {
                if !same_code {
                    return Self::Package(first_package_id.unwrap());
                } else {
                    return Self::Mixed {
                        package_id: first_package_id.unwrap(),
                        code,
                    };
                }
            }

            if same_code {
                return Self::Code(code);
            }
        }

        Self::Any
    }

    pub fn allows_card(self, card: &Card) -> bool {
        match self {
            Self::Code(code) => (card.code == code && code != "!") || card.code == "*",
            Self::Package(package_id) => card.package_id == *package_id,
            Self::Mixed { package_id, code } => {
                card.package_id == *package_id || card.code == code || card.code == "*"
            }
            Self::Any => true,
        }
    }
}
