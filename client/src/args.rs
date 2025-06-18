use clap::Parser;

#[derive(Parser)]
pub struct Args {
    #[clap(short, long, value_parser, default_value = "0")]
    pub port: u16,
    #[clap(long, value_parser, default_value = "65536")]
    pub resend_budget: usize,
    #[clap(long)]
    pub shared_data_folder: bool,
}
