pub fn parse_rgb_table(table: mlua::Table) -> mlua::Result<(u8, u8, u8)> {
    let color = if let Some(r) = table.get::<_, Option<u8>>("r")? {
        (r, table.get("g")?, table.get("b")?)
    } else {
        (table.get(1)?, table.get(2)?, table.get(3)?)
    };

    Ok(color)
}

pub fn parse_rgba_table(table: mlua::Table) -> mlua::Result<(u8, u8, u8, u8)> {
    let color = if let Some(r) = table.get::<_, Option<u8>>("r")? {
        (
            r,
            table.get("g")?,
            table.get("b")?,
            table.get::<_, Option<u8>>("a")?.unwrap_or(255),
        )
    } else {
        (
            table.get(1)?,
            table.get(2)?,
            table.get(3)?,
            table.get::<_, Option<u8>>(4)?.unwrap_or(255),
        )
    };

    Ok(color)
}
