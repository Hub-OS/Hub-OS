use clap::Parser;

#[derive(Parser, Debug, Clone)]
pub struct Args {
    pub folder_a: String,
    pub folder_b: String,
}

fn main() {
    let args = Args::parse();
    let mut i = 1;

    if !std::fs::exists(&args.folder_a).unwrap_or_default() {
        println!("{} does not exist", args.folder_a);
        return;
    }

    if !std::fs::exists(&args.folder_b).unwrap_or_default() {
        println!("{} does not exist", args.folder_b);
        return;
    }

    loop {
        let paths = [
            format!("{}/{i}.png", args.folder_a),
            format!("{}/{i}.png", args.folder_b),
        ];

        let images = paths.map(|path| {
            let contents = std::fs::read(path).ok()?;
            image::load_from_memory(&contents).ok()
        });

        let diff = images[0] != images[1];

        if diff {
            println!("Difference starts at {i}.png");
            break;
        }

        if images.iter().all(|image| image.is_none()) {
            println!("No difference");
            break;
        }

        i += 1;
    }
}
