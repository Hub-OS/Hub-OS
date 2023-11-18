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
        let deck_indices = player.staged_items.selected_deck_card_indices();

        if deck_indices.is_empty() {
            return Self::Any;
        }

        let mut first_package_id: Option<&PackageId> = None;
        let mut code_restriction: Option<&str> = None;

        let mut same_package_id = true;
        let mut same_code = true;

        for i in deck_indices {
            let card = &player.deck[i];

            if let Some(package_id) = first_package_id {
                same_package_id &= card.package_id == *package_id;
            } else {
                first_package_id = Some(&card.package_id);
            }

            if let Some(code) = code_restriction {
                same_code &= card.code == code || card.code == "*";
            } else if card.code != "*" {
                code_restriction = Some(&card.code);
            }
        }

        if let Some(code) = code_restriction {
            if same_package_id {
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
            Self::Code(code) => card.code == code || card.code == "*",
            Self::Package(package_id) => card.package_id == *package_id,
            Self::Mixed { package_id, code } => {
                card.package_id == *package_id || card.code == code || card.code == "*"
            }
            Self::Any => true,
        }
    }
}
