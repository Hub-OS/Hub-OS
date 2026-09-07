use framework::common::{GameIO, GameService};
use framework::logging::LogRecord;
use std::collections::VecDeque;
use std::io::BufWriter;

const MAX_HISTORY: usize = 30;

#[derive(Default)]
pub struct Logs {
    ring: VecDeque<LogRecord>,
    historic_count: usize,
}

impl Logs {
    pub fn iter_all(&self) -> impl Iterator<Item = &LogRecord> {
        self.ring.iter()
    }

    pub fn iter_old(&self) -> impl Iterator<Item = &LogRecord> {
        self.ring.iter().take(self.historic_count)
    }

    pub fn iter_new(&self) -> impl Iterator<Item = &LogRecord> {
        self.ring.iter().skip(self.historic_count)
    }
}

pub struct LogService {
    log_receiver: flume::Receiver<LogRecord>,
    log_buffer: Option<BufWriter<std::fs::File>>,
}

impl LogService {
    pub fn new(
        game_io: &mut GameIO,
        log_path: Option<String>,
        log_receiver: flume::Receiver<LogRecord>,
    ) -> Self {
        game_io.set_resource(Logs::default());

        Self {
            log_receiver,
            log_buffer: log_path
                .and_then(|path| std::fs::File::create(path).ok())
                .map(BufWriter::new),
        }
    }
}

impl GameService for LogService {
    fn pre_update(&mut self, game_io: &mut GameIO) {
        let logs = game_io.resource_mut::<Logs>().unwrap();

        logs.historic_count = logs.ring.len();

        while let Ok(record) = self.log_receiver.try_recv() {
            if logs.ring.len() >= MAX_HISTORY && logs.historic_count > 0 {
                logs.ring.pop_front();
                logs.historic_count -= 1;
            }

            logs.ring.push_back(record);
        }

        if let Some(writer) = &mut self.log_buffer {
            use std::io::Write;

            for record in logs.iter_new() {
                let _ = writeln!(
                    writer,
                    "{} [{}] {}",
                    record.level, record.target, record.message
                );
            }

            let _ = writer.flush();
        }
    }
}
