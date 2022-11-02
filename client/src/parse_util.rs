pub fn parse_or_default<T>(opt_str: Option<&str>) -> T
where
    T: Default + std::str::FromStr,
{
    opt_str
        .map(|s| s.parse::<T>().unwrap_or_default())
        .unwrap_or_default()
}
