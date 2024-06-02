use std::collections::HashSet;
use std::ffi::c_void;

pub fn optional_lua_string_to_optional_str<'a>(
    optional_string: &'a Option<mlua::String>,
) -> mlua::Result<Option<&'a str>> {
    optional_string
        .as_ref()
        .map(|lua_string| lua_string.to_str())
        .transpose()
}

pub fn optional_lua_string_to_str<'a>(
    optional_string: &'a Option<mlua::String>,
) -> mlua::Result<&'a str> {
    Ok(optional_lua_string_to_optional_str(optional_string)?.unwrap_or_default())
}

pub fn lua_value_to_string(
    value: mlua::Value,
    indentation: &str,
    indentation_level: usize,
) -> String {
    let mut visited = HashSet::new();
    lua_value_to_string_internal(value, indentation, indentation_level, &mut visited)
}

fn lua_value_to_string_internal(
    value: mlua::Value,
    indentation: &str,
    indentation_level: usize,
    visited: &mut HashSet<*const c_void>,
) -> String {
    match value {
        mlua::Value::Table(table) => {
            let key = table.to_pointer();
            if !visited.insert(key) {
                return String::from("Circular Reference");
            }

            let pair_strings: Vec<String> = table
                .pairs()
                .map(|pair: mlua::Result<(mlua::Value, mlua::Value)>| {
                    let (key, value) = pair.unwrap();

                    format!(
                        "{}[{}] = {}",
                        indentation,
                        lua_value_to_string_internal(
                            key,
                            indentation,
                            indentation_level + 1,
                            visited
                        ),
                        lua_value_to_string_internal(
                            value,
                            indentation,
                            indentation_level + 1,
                            visited
                        ),
                    )
                })
                .collect();

            visited.remove(&key);

            if indentation.is_empty() {
                // {pair_strings}, `{{` and `}}` are escaped versions of `{` and `}`
                format!("{{{}}}", pair_strings.join(","))
            } else {
                let indentation_string = indentation.repeat(indentation_level);

                let separator_string = String::from(",\n") + &indentation.repeat(indentation_level);

                format!(
                    "{{\n{}{}\n{}}}",
                    indentation_string,
                    pair_strings.join(&separator_string),
                    indentation_string
                )
            }
        }
        mlua::Value::String(lua_string) => format!(
            // wrap with ""
            "{:?}",
            // escape "
            String::from_utf8_lossy(lua_string.as_bytes()).replace('"', "\\\"")
        ),
        mlua::Value::Number(n) => n.to_string(),
        mlua::Value::Integer(i) => i.to_string(),
        mlua::Value::Boolean(b) => b.to_string(),
        mlua::Value::Nil => String::from("nil"),
        // these will create errors
        mlua::Value::Function(_) => String::from("Function"),
        mlua::Value::Thread(_) => String::from("Thread"),
        mlua::Value::LightUserData(_) => String::from("LightUserData"),
        mlua::Value::UserData(_) => String::from("UserData"),
        mlua::Value::Error(error) => format!("{:?}", error),
    }
}
