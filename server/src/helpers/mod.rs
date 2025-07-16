pub fn normalize_path(path: &std::path::Path) -> std::path::PathBuf {
    let mut normalized_path: std::path::PathBuf = std::path::PathBuf::new();

    for component in path.components() {
        let component_as_os_str = component.as_os_str();

        if component_as_os_str == "." {
            continue;
        }

        if component_as_os_str == ".." {
            if normalized_path.file_name().is_none() {
                // file_name() returns none for ".." as last component and no component
                // this means we're building out the start like ../../../etc
                normalized_path.push("..");
            } else {
                normalized_path.pop();
            }
        } else {
            normalized_path.push(component);
        }
    }

    normalized_path
}
