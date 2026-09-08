use itertools::Itertools;
use std::fs::File;
use std::io::{Cursor, Read, Write};
use std::path::Path;
use walkdir::WalkDir;
use zip::ZipWriter;
use zip::read::ZipFile;
use zip::result::ZipResult;
use zip::write::FileOptions as ZipFileOptions;

pub fn clean_path(path_str: &str) -> String {
    let path = path_clean::clean(path_str.replace('\\', "/"))
        .to_str()
        .unwrap_or_default()
        .replace('\\', "/");

    path.strip_prefix("./").map(String::from).unwrap_or(path)
}

fn parent(path_str: &str) -> Option<&str> {
    let last_index = path_str.len().max(1) - 1;
    let slash_index = path_str[..last_index].rfind('/')?;

    Some(&path_str[..slash_index + 1])
}

pub fn extract(bytes: &[u8], mut file_callback: impl FnMut(String, ZipFile)) {
    let cursor = Cursor::new(bytes);
    let mut archive = match zip::ZipArchive::new(cursor) {
        Ok(archive) => archive,
        Err(err) => {
            log::error!("Failed to read zip: {err}");
            return;
        }
    };

    for i in 0..archive.len() {
        let file = match archive.by_index(i) {
            Ok(file) => file,
            Err(err) => {
                log::error!("Failed to grab a file within zip: {err}");
                continue;
            }
        };

        let Some(path) = file.enclosed_name().and_then(|path| path.to_str()) else {
            log::error!("Invalid file name within zip {:?}", file.name());
            continue;
        };
        let path = clean_path(path);

        file_callback(path, file);
    }
}

pub fn extract_to(bytes: &[u8], base_path: &str) {
    let _ = std::fs::remove_dir_all(base_path);

    extract(bytes, |path, mut virtual_file| {
        let path = format!("{base_path}{path}");

        if let Some(parent_path) = parent(&path)
            && let Err(err) = std::fs::create_dir_all(parent_path)
        {
            log::error!("Failed to create directory {parent_path:?}: {err}");
        }

        let res = std::fs::File::create(&path)
            .and_then(|mut file| std::io::copy(&mut virtual_file, &mut file));

        if let Err(err) = res {
            log::error!("Failed to write to {path:?}: {err}");
        }
    });
}

pub fn compress<S>(path: &S) -> ZipResult<Vec<u8>>
where
    S: AsRef<Path>,
{
    let file_options = ZipFileOptions::default();

    let mut data = Vec::new();

    {
        let cursor = Cursor::new(&mut data);
        let mut zip_writer = ZipWriter::new(cursor);
        let mut buffer = Vec::new();

        let root_path = path;

        let entry_iter = WalkDir::new(path)
            .into_iter()
            .flatten()
            .map(|entry| {
                let stripped_path = entry.path().strip_prefix(root_path).unwrap();
                let cleaned_path = clean_path(&stripped_path.to_string_lossy());

                (cleaned_path, entry)
            })
            // skipping anything that isn't a file
            .filter(|(_, entry)| entry.metadata().is_ok_and(|entry| entry.is_file()))
            // skipping hidden files, also filter out anything that can't be represented by UTF-8
            // this should strip out .vscode folders and files in the __MACOSX folder
            .filter(|(_, entry)| {
                let Ok(stripped_path) = entry.path().strip_prefix(root_path) else {
                    return false;
                };

                let mut path_iter = stripped_path.iter();
                path_iter.all(|slice| slice.to_str().is_some_and(|s| !is_hidden_file(s)))
            })
            .sorted_by(|(path_a, _), (path_b, _)| path_a.cmp(path_b));

        for (cleaned_path, entry) in entry_iter {
            let Ok(mut file) = File::open(entry.path()) else {
                continue;
            };

            let _ = file.read_to_end(&mut buffer);

            let _ = zip_writer.start_file(cleaned_path, file_options);
            let _ = zip_writer.write_all(&buffer);

            buffer.clear();
        }

        zip_writer.finish()?;
    }

    Ok(data)
}

fn is_hidden_file(file_name: &str) -> bool {
    file_name.starts_with('.')
}
