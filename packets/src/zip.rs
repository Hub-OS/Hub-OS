use itertools::Itertools;
use std::fs::File;
use std::io::{Cursor, Read, Write};
use std::path::Path;
use walkdir::WalkDir;
use zip::read::ZipFile;
use zip::result::ZipResult;
use zip::write::FileOptions as ZipFileOptions;
use zip::ZipWriter;

pub fn clean_path(path_str: &str) -> String {
    let path = path_clean::clean(path_str)
        .to_str()
        .unwrap_or_default()
        .replace('\\', "/");

    path.strip_prefix("./").map(String::from).unwrap_or(path)
}

pub fn extract(bytes: &[u8], mut file_callback: impl FnMut(String, ZipFile)) {
    let cursor = Cursor::new(bytes);
    let mut archive = match zip::ZipArchive::new(cursor) {
        Ok(archive) => archive,
        Err(err) => {
            log::error!("Failed to read zip: {}", err);
            return;
        }
    };

    for i in 0..archive.len() {
        let file = match archive.by_index(i) {
            Ok(file) => file,
            Err(err) => {
                log::error!("Failed to grab a file within zip: {}", err);
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
            .sorted_by_cached_key(|entry| entry.path().to_path_buf());

        for entry in entry_iter {
            let Ok(metadata) = entry.metadata() else {
                continue;
            };

            if metadata.is_dir() {
                // would just waste space
                continue;
            }

            let Ok(mut file) = File::open(entry.path()) else {
                continue;
            };

            let _ = file.read_to_end(&mut buffer);

            let stripped_path = entry.path().strip_prefix(root_path).unwrap();
            let cleaned_path = clean_path(&stripped_path.to_string_lossy());

            let _ = zip_writer.start_file(cleaned_path, file_options);
            let _ = zip_writer.write_all(&buffer);

            buffer.clear();
        }

        zip_writer.finish()?;
    }

    Ok(data)
}
