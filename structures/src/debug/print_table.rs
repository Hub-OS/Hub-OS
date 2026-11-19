fn measure_graphemes(s: &str) -> usize {
    use unicode_segmentation::UnicodeSegmentation;
    s.graphemes(true).count()
}

pub struct PrintTableColumn<'f, 'label, T> {
    pub label: &'label str,
    pub measure: Box<dyn Fn(T) -> usize + 'f>,
    pub write: Box<dyn for<'a> Fn(&mut dyn std::fmt::Write, usize, &'a T) -> std::fmt::Result + 'f>,
}

impl<'f, 'label, T> PrintTableColumn<'f, 'label, T> {
    pub fn new(
        label: &'label str,
        measure: impl Fn(T) -> usize + 'f,
        write: impl for<'a> Fn(&mut dyn std::fmt::Write, usize, &'a T) -> std::fmt::Result + 'f,
    ) -> Self {
        Self {
            label,
            measure: Box::new(measure),
            write: Box::new(write),
        }
    }

    pub fn new_str<F: Clone + Fn(&T) -> &str + 'f>(label: &'label str, map: F) -> Self {
        let measure_map = map.clone();

        Self {
            label,
            measure: Box::new(move |v| measure_graphemes(measure_map(&v))),
            write: Box::new(move |s, _, v| write!(s, "{}", map(v))),
        }
    }

    pub fn new_checkmark<F: Fn(&T) -> bool + 'f>(
        label: &'label str,
        checkmark: &'f str,
        test: F,
    ) -> Self {
        let w = measure_graphemes(checkmark);

        Self {
            label,
            measure: Box::new(move |_| w),
            write: Box::new(move |s, width, v| {
                if test(v) {
                    let extra_space = width.saturating_sub(w);
                    let left = extra_space.div_ceil(2);
                    let right = extra_space / 2;

                    write!(s, "{:left$}{}{:right$}", "", checkmark, "")
                } else {
                    Ok(())
                }
            }),
        }
    }

    pub fn new_usize<F: Clone + Fn(&T) -> usize + 'f>(label: &'label str, map: F) -> Self {
        let measure_map = map.clone();

        Self {
            label,
            measure: Box::new(move |v| {
                (measure_map(&v).checked_ilog10().unwrap_or_default() + 1) as _
            }),
            write: Box::new(move |s, width, v| write!(s, "{:>width$}", map(v))),
        }
    }
}

pub fn print_table<'a, T: 'a, I: Iterator<Item = T>>(
    iter: impl Fn() -> I,
    columns: &[PrintTableColumn<T>],
) {
    let widths: Vec<_> = columns
        .iter()
        .map(|column| {
            let label_len = measure_graphemes(column.label);
            let longest_value = iter().map(|value| (column.measure)(value)).max();

            longest_value.unwrap_or_default().max(label_len)
        })
        .collect();

    // print header
    println!();

    for (column, width) in columns.iter().zip(&widths) {
        print!("| {:<width$} ", column.label);
    }

    println!("|");

    // print divider
    for width in &widths {
        print!("| {:-<width$} ", "");
    }

    println!("|");

    // print data
    let mut row = String::with_capacity(columns.len() * 2 + widths.iter().sum::<usize>() + 2);

    for value in iter() {
        for (column, width) in columns.iter().zip(&widths) {
            row.push_str("| ");

            let prev_len = row.len();

            let _ = (column.write)(&mut row, *width, &value);

            // ensure min len
            let used_space = measure_graphemes(&row[prev_len..]);
            let remaining_space = width.saturating_sub(used_space);

            // adding 1 to include padding
            for _ in 0..remaining_space + 1 {
                row.push(' ');
            }
        }

        println!("{row}|");
        row.clear();
    }

    println!();
}
