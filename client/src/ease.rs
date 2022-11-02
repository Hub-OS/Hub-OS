pub fn symmetric(power: f32, progress: f32) -> f32 {
    let x = progress;

    // 1 - |(x * 2.0 - 1)^power|
    // plug it into a graphing tool to help understand
    // abs() prevents odd numbers from breaking symmetry
    let y = 1.0 - (x * 2.0 - 1.0).powf(power).abs();

    // prevent negative values
    y.max(0.0)
}

pub fn quadratic(progress: f32) -> f32 {
    symmetric(2.0, progress)
}

/// returns progress from value \[0.0, 1.0\]
macro_rules! inverse_lerp {
    ($start:expr, $end:expr, $value:expr) => {{
        let start = $start as f32;
        let end = $end as f32;
        let value = $value as f32;

        let range = end - start;
        ((value - start) / range).clamp(0.0, 1.0)
    }};
}

pub(crate) use inverse_lerp;
