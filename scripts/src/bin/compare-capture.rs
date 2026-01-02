use clap::Parser;

#[derive(Parser, Debug, Clone)]
pub struct Args {
    pub folder_a: String,
    pub folder_b: String,
    /// Lists every frame that differs instead of just the first frame
    #[arg(long)]
    pub all: bool,
}

fn main() {
    let args = Args::parse();
    let mut i = 1;
    let mut differences = 0;

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
            if !args.all {
                println!("Difference starts at {i}.png");
                break;
            }

            println!("Difference at {i}.png");
            differences += 1;
        }

        if images.iter().all(|image| image.is_none()) {
            if differences == 0 {
                println!("No difference");
            } else {
                println!("{differences} difference(s)");
            }
            break;
        }

        i += 1;
    }
}
