use std::fs::File;
use std::io::{Cursor, Read, Write};
use std::path::Path;
use walkdir::WalkDir;
use zip::read::ZipFile;
use zip::result::ZipResult;
use zip::write::FileOptions as ZipFileOptions;
use zip::{CompressionMethod, ZipWriter};

use super::ResourcePaths;

pub fn extract(bytes: &[u8], mut file_callback: impl FnMut(String, ZipFile)) {
    let cursor = Cursor::new(bytes);
    let mut archive = match zip::ZipArchive::new(cursor) {
        Ok(archive) => archive,
        Err(err) => {
            log::error!("failed to read zip: {}", err);
            return;
        }
    };

    for i in 0..archive.len() {
        let file = match archive.by_index(i) {
            Ok(file) => file,
            Err(err) => {
                log::error!("failed to grab a file within zip: {}", err);
                continue;
            }
        };

        let path = match file.enclosed_name().and_then(|path| path.to_str()) {
            Some(path) => ResourcePaths::clean(path),
            None => {
                log::error!("invalid file name within zip {:?}", file.name());
                continue;
            }
        };

        file_callback(path, file);
    }
}

pub fn compress<S>(path: &S) -> ZipResult<Vec<u8>>
where
    S: AsRef<Path>,
{
    let file_options = ZipFileOptions::default().compression_method(CompressionMethod::Zstd);

    let mut data = Vec::new();

    {
        let cursor = Cursor::new(&mut data);
        let mut zip_writer = ZipWriter::new(cursor);
        let mut buffer = Vec::new();

        let root_path = path;

        for entry in WalkDir::new(&path).into_iter().flatten() {
            let metadata = match entry.metadata() {
                Ok(metadata) => metadata,
                _ => continue,
            };

            if metadata.is_dir() {
                // would just waste space
                continue;
            }

            let mut file = match File::open(entry.path()) {
                Ok(file) => file,
                _ => continue,
            };

            let _ = file.read_to_end(&mut buffer);

            let stripped_path = entry.path().strip_prefix(root_path).unwrap();
            let cleaned_path = ResourcePaths::clean(&stripped_path.to_string_lossy());

            let _ = zip_writer.start_file(cleaned_path, file_options);
            let _ = zip_writer.write_all(&buffer);

            buffer.clear();
        }

        zip_writer.finish()?;
    }

    Ok(data)
}
