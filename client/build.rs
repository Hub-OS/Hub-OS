// https://github.com/RustAudio/rodio/issues/404#issuecomment-1288096846

use std::env;

fn main() {
    let target_os = env::var("CARGO_CFG_TARGET_OS").unwrap();

    #[allow(clippy::single_match)]
    match target_os.as_str() {
        "android" => {
            println!("cargo:rustc-link-lib=dylib=stdc++");
            println!("cargo:rustc-link-lib=c++_shared");
        }
        _ => {}
    }
}
