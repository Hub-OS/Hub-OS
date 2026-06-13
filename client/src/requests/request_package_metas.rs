use crate::requests::PackageResponse;
use packets::address_parsing::uri_encode;
use packets::structures::PackageCategory;
use std::fmt::Write;
use strum::EnumIter;

pub struct RequestPackageMetaOptions<'s> {
    pub skip: usize,
    pub limit: usize,
    pub name_filter: &'s str,
    pub category_filter: PackageCategoryFilter,
}

#[derive(Default, EnumIter, Clone, Copy, PartialEq, Eq)]
pub enum PackageCategoryFilter {
    #[default]
    All,
    Cards,
    Augments,
    Encounters,
    Players,
    Resource,
    Packs,
}

impl PackageCategoryFilter {
    pub fn translation_key(&self) -> &'static str {
        match self {
            PackageCategoryFilter::All => "packages-category-filter-all",
            PackageCategoryFilter::Cards => "packages-category-filter-cards",
            PackageCategoryFilter::Augments => "packages-category-filter-augments",
            PackageCategoryFilter::Encounters => "packages-category-filter-encounters",
            PackageCategoryFilter::Players => "packages-category-filter-players",
            PackageCategoryFilter::Resource => "packages-category-filter-resource",
            PackageCategoryFilter::Packs => "packages-category-filter-packs",
        }
    }

    fn value(&self) -> &'static str {
        match self {
            PackageCategoryFilter::All => "",
            PackageCategoryFilter::Cards => "card",
            PackageCategoryFilter::Augments => "augment",
            PackageCategoryFilter::Encounters => "encounter",
            PackageCategoryFilter::Players => "player",
            PackageCategoryFilter::Resource => "resource",
            PackageCategoryFilter::Packs => "pack",
        }
    }

    pub fn package_category(&self) -> Option<PackageCategory> {
        match self {
            PackageCategoryFilter::All => None,
            PackageCategoryFilter::Cards => Some(PackageCategory::Card),
            PackageCategoryFilter::Augments => Some(PackageCategory::Augment),
            PackageCategoryFilter::Encounters => Some(PackageCategory::Encounter),
            PackageCategoryFilter::Players => Some(PackageCategory::Player),
            PackageCategoryFilter::Resource => Some(PackageCategory::Resource),
            PackageCategoryFilter::Packs => Some(PackageCategory::Library),
        }
    }
}

pub fn request_package_metas(
    repo: &str,
    options: RequestPackageMetaOptions,
) -> impl Future<Output = Option<Vec<PackageResponse>>> + use<> {
    let mut uri = format!("{repo}/api/mods?limit={}", options.limit);

    if options.skip > 0 {
        let _ = write!(&mut uri, "&skip={}", options.skip);
    }

    let category_str = options.category_filter.value();
    if !category_str.is_empty() {
        let _ = write!(&mut uri, "&category={category_str}");
    }

    if !options.name_filter.is_empty() {
        let _ = write!(&mut uri, "&name={}", uri_encode(options.name_filter));
    }

    async move { super::request_json(&uri).await }
}
