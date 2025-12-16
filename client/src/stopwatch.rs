// Used for debugging. Example usage:
// ```
// use crate::stopwatch::Stopwatch;
// let stopwatch = Stopwatch::start_new();
//
// task();
// stopwatch.mark("task()");
//
// stopwatch.print_percentages();
// ```

use indexmap::IndexMap;
use std::time::{Duration, Instant};

pub struct Stopwatch {
    markers: Vec<(&'static str, Instant)>,
}

impl Stopwatch {
    pub fn start_new() -> Self {
        Self {
            markers: vec![("", Instant::now())],
        }
    }

    /// The duration tied to this mark is the duration since the last mark
    pub fn mark(&mut self, label: &'static str) {
        self.markers.push((label, Instant::now()));
    }

    /// Returns the duration between the call to ::start_new() and the last ::mark() call
    pub fn total_duration(&self) -> Duration {
        let final_instant = self.markers.last().unwrap().1;
        let start_instant = self.markers[0].1;

        final_instant - start_instant
    }

    /// Prints the percentage of the total duration used by each marker, deduplicated
    pub fn print_percentages(&self) {
        let total_duration_secs = self.total_duration().as_secs_f32();

        self.for_each_deduped(|label, duration| {
            println!(
                "{label} - {:.2}%",
                duration.as_secs_f32() / total_duration_secs * 100.0
            );
        });
    }

    #[inline]
    pub fn for_each_deduped(&self, mut callback: impl FnMut(&'static str, Duration)) {
        self.for_each_deduped_impl(&mut callback)
    }

    fn for_each_deduped_impl(&self, callback: &mut dyn FnMut(&'static str, Duration)) {
        let mut prev_instant = self.markers[0].1;

        let mut deduped = IndexMap::<&'static str, Duration>::new();

        for &(label, instant) in &self.markers[1..] {
            *deduped.entry(label).or_default() += instant - prev_instant;
            prev_instant = instant;
        }

        for (label, duration) in deduped {
            callback(label, duration);
        }
    }
}
