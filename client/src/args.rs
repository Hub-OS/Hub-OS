use clap::Parser;

#[derive(Parser)]
pub struct Args {
    #[clap(long, value_parser, default_value = "1310720")]
    pub send_budget: usize,
    #[clap(long)]
    pub data_folder: Option<String>,
    #[clap(long)]
    pub record_rollbacks: bool,
    #[clap(long)]
    pub replay_rollbacks: bool,
}
