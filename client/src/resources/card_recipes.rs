use crate::bindable::CardProperties;
use crate::packages::{CardPackage, PackageNamespace};
use packets::structures::PackageId;
use std::collections::{HashMap, HashSet};
use std::ops::Range;

#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub enum CardRecipeIdType {
    Name,
    Id,
}

impl CardRecipeIdType {
    fn resolve_test_fn(&self) -> impl Fn(&str, &CardProperties) -> bool {
        match self {
            CardRecipeIdType::Id => {
                |id: &str, card: &CardProperties| card.package_id.as_str() == id
            }
            CardRecipeIdType::Name => |name: &str, card: &CardProperties| card.short_name == *name,
        }
    }
}

#[derive(Clone)]
pub enum CardRecipe {
    CodeSequence {
        id_type: CardRecipeIdType,
        id: String,
        codes: Vec<String>,
    },
    MixSequence {
        mix: Vec<(CardRecipeIdType, String)>,
        ordered: bool,
    },
}

impl CardRecipe {
    pub fn from_toml(recipe: &toml::Value) -> Option<Self> {
        if let Some(list) = recipe.get("mix").and_then(|v| v.as_array()) {
            let mut mix = Vec::new();

            for value in list {
                if let Some(name) = value.get("name").and_then(|v| v.as_str()) {
                    mix.push((CardRecipeIdType::Name, name.to_string()))
                } else if let Some(id) = value.get("id").and_then(|v| v.as_str()) {
                    mix.push((CardRecipeIdType::Id, id.to_string()))
                } else {
                    log::error!("Missing name or id in mix recipe list");
                    return None;
                }
            }

            let ordered = recipe
                .get("ordered")
                .and_then(|v| v.as_bool())
                .unwrap_or(true);

            return Some(Self::MixSequence { mix, ordered });
        }

        if let Some(list) = recipe.get("codes").and_then(|v| v.as_array()) {
            let mut codes = Vec::new();
            let id_type;
            let id;

            if let Some(name) = recipe.get("name").and_then(|v| v.as_str()) {
                id_type = CardRecipeIdType::Name;
                id = name.to_string();
            } else if let Some(id_value) = recipe.get("id").and_then(|v| v.as_str()) {
                id_type = CardRecipeIdType::Id;
                id = id_value.to_string();
            } else {
                log::error!("Missing name or id in codes recipe");
                return None;
            }

            for value in list {
                let Some(code) = value.as_str() else {
                    log::error!("Expecting codes recipe list to contain only strings");
                    return None;
                };

                codes.push(code.to_string());
            }

            return Some(Self::CodeSequence { id_type, id, codes });
        }

        log::error!("Recipe requires a mix or codes field");
        None
    }
}

#[derive(Default)]
struct NamespaceData {
    /// (name or id of first input) -> Vec<(Output, Recipe, limit)>
    recipes: HashMap<String, Vec<(PackageId, CardRecipe, usize)>>,
    /// package_id -> HashSet<name or id of first input>
    tracked_output: HashMap<PackageId, HashSet<String>>,
}

#[derive(Default)]
pub struct CardRecipes {
    namespace_map: HashMap<PackageNamespace, NamespaceData>,
}

impl CardRecipes {
    pub fn load_from_package(&mut self, namespace: PackageNamespace, package: &CardPackage) {
        let info = &package.package_info;

        let data = self.namespace_map.entry(namespace).or_default();
        let tracking = data.tracked_output.entry(info.id.clone()).or_default();

        let mut push = |start_id: String, recipe: &CardRecipe| {
            let recipe_list = data.recipes.entry(start_id.clone()).or_default();
            recipe_list.push((info.id.clone(), recipe.clone(), package.limit));
            recipe_list.sort_by(|(a, ..), (b, ..)| a.cmp(b));

            tracking.insert(start_id);
        };

        for recipe in &package.recipes {
            match recipe {
                CardRecipe::CodeSequence { id, .. } => {
                    push(id.clone(), recipe);
                }
                CardRecipe::MixSequence {
                    mix: sequence,
                    ordered,
                } => {
                    if *ordered {
                        if let Some((_, id)) = sequence.first() {
                            push(id.clone(), recipe);
                        }
                    } else {
                        // add all unique cards as a potential start if this works in any order
                        let mut added = HashSet::new();

                        for (_, id) in sequence {
                            if added.insert(id) {
                                push(id.clone(), recipe);
                            }
                        }
                    }
                }
            };
        }
    }

    pub fn remove_associated(&mut self, namespace: PackageNamespace, output: &PackageId) {
        let Some(data) = self.namespace_map.get_mut(&namespace) else {
            return;
        };

        let Some(tracking) = data.tracked_output.remove(output) else {
            return;
        };

        for name_or_id in tracking {
            let Some(recipe_list) = data.recipes.get_mut(&name_or_id) else {
                continue;
            };

            recipe_list.retain(|(o, ..)| o != output);
        }
    }

    pub fn remove_namespace(&mut self, namespace: PackageNamespace) {
        self.namespace_map.remove(&namespace);
    }

    pub fn related_output<'a>(
        &'a self,
        namespace: PackageNamespace,
        package: &'a CardPackage,
    ) -> Vec<PackageId> {
        let Some(data) = self.namespace_map.get(&namespace) else {
            return Default::default();
        };

        let mut results = Vec::new();

        let id_result = data.recipes.get(package.package_info.id.as_str());
        let name_result = data.recipes.get(&*package.card_properties.short_name);

        let iter = id_result.into_iter().chain(name_result).flatten();

        for (output, ..) in iter {
            if !results.contains(output) {
                results.push(output.clone())
            }
        }

        results
    }

    pub fn resolve_changes(
        &self,
        namespace: PackageNamespace,
        cards: &[CardProperties],
        use_counts: &HashMap<PackageId, usize>,
    ) -> Vec<(PackageId, Range<usize>)> {
        let mut results = Vec::new();

        let Some(data) = self.namespace_map.get(&namespace) else {
            // no recipes
            return results;
        };

        let recipe_map = &data.recipes;

        let mut i = 0;

        while let Some(card) = cards.get(i) {
            let id_result = recipe_map.get(card.package_id.as_str());
            let name_result = recipe_map.get(&*card.short_name);

            let iter = id_result
                .into_iter()
                .chain(name_result.into_iter())
                .flatten();

            let mut recipe_output = None;
            let mut recipe_len = 0;
            let remaining_cards = &cards[i..];

            'recipe_loop: for (output, recipe, limit) in iter {
                if use_counts.get(output).is_some_and(|uses| uses >= limit) {
                    continue;
                }

                match recipe {
                    CardRecipe::CodeSequence { id_type, id, codes } => {
                        if codes.len() < recipe_len {
                            // already found a better match
                            continue;
                        }

                        let test_id = id_type.resolve_test_fn();

                        if codes.len() > remaining_cards.len() {
                            // not enough cards, fail recipe
                            continue;
                        }

                        let mut used_asterisk = false;

                        for (card, code) in remaining_cards.iter().zip(codes) {
                            if !test_id(id.as_str(), card) {
                                // card doesn't match id, fail recipe
                                continue 'recipe_loop;
                            }

                            if card.code == *code {
                                // card matches code, continue testing
                                continue;
                            }

                            if card.code != "*" || used_asterisk {
                                // card isn't asterisk or already used an asterisk, fail recipe
                                continue 'recipe_loop;
                            }

                            used_asterisk = true;
                        }

                        recipe_len = codes.len();
                    }
                    CardRecipe::MixSequence { mix, ordered } => {
                        if mix.len() < recipe_len {
                            // already found a better match
                            continue;
                        }

                        if mix.len() > remaining_cards.len() {
                            // not enough cards, fail recipe
                            continue;
                        }

                        if *ordered {
                            for (card, (id_type, id)) in remaining_cards.iter().zip(mix) {
                                let test_id = id_type.resolve_test_fn();

                                if !test_id(id.as_str(), card) {
                                    // card doesn't match id, fail recipe
                                    continue 'recipe_loop;
                                }
                            }
                        } else {
                            let mut still_required: Vec<_> = mix.iter().collect();

                            for card in remaining_cards.iter() {
                                let Some(index) =
                                    still_required.iter().position(|(id_type, id)| {
                                        let test_id = id_type.resolve_test_fn();
                                        test_id(id.as_str(), card)
                                    })
                                else {
                                    // couldn't find a match, fail recipe
                                    continue 'recipe_loop;
                                };

                                still_required.swap_remove(index);

                                if still_required.is_empty() {
                                    // completed
                                    break;
                                }
                            }
                        }

                        recipe_len = mix.len();
                    }
                }

                recipe_output = Some(output);
            }

            if let Some(output) = recipe_output {
                results.push((output.clone(), i..i + recipe_len))
            }

            // skip cards in the recipe, or at least move on to the next card
            i += recipe_len.max(1);
        }

        results
    }
}
