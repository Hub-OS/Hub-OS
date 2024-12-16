pub fn parse_or_default<T>(opt_str: Option<&str>) -> T
where
    T: Default + std::str::FromStr,
{
    opt_str
        .map(|s| s.parse::<T>().unwrap_or_default())
        .unwrap_or_default()
}

pub fn parse_or<T>(opt_str: Option<&str>, default: T) -> T
where
    T: Default + std::str::FromStr + Copy,
{
    let Some(s) = opt_str else {
        return default;
    };

    s.parse::<T>().unwrap_or(default)
}
