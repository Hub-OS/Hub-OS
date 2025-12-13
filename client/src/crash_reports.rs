use crate::resources::CRASH_REPORT_ENDPOINT;
use std::collections::HashMap;
use std::sync::{LazyLock, Mutex};

static CONTEXT: LazyLock<Mutex<HashMap<String, String>>> = LazyLock::new(Default::default);

pub fn set_crash_context(key: &str, context: String) {
    let mut map = CONTEXT.lock().unwrap();
    map.insert(key.to_string(), context);
}

pub fn take_crash_context(key: &str) -> Option<String> {
    let mut map = CONTEXT.lock().unwrap();
    map.remove(key)
}

fn create_context_string() -> String {
    let map = CONTEXT.lock().unwrap();

    if map.is_empty() {
        return String::new();
    }

    let mut context_string = String::from("Context:\n");

    for (i, (key, value)) in map.iter().enumerate() {
        if i != 0 {
            context_string.push('\n');
        }

        context_string.push_str("  ");
        context_string.push_str(key);
        context_string.push_str(": ");
        context_string.push_str(value);
    }

    context_string
}

pub fn print_crash_report(mut message: String) {
    let context_string = create_context_string();

    if !context_string.is_empty() {
        // adding context to the bottom, as it's easier to see in a terminal
        message = message + "\n" + &context_string + "\n";
    }

    eprintln!("{message}");
}

pub fn send_crash_report(mut message: String) {
    if cfg!(debug_assertions) {
        println!("\nAutomated crash reports are disabled in debug builds.");
        return;
    }

    let context_string = create_context_string();

    if !context_string.is_empty() {
        // adding context to the top, as it's easier to see on the website
        message = context_string + "\n\n" + &message;
    }

    let request = minreq::post(CRASH_REPORT_ENDPOINT).with_body(message);

    if request
        .send()
        .map(|response| response.status_code == 200)
        .unwrap_or_default()
    {
        println!("\nCrash report uploaded to hubos.dev. Give the devs a heads up.");
    } else {
        println!("\nFailed to send crash report. Share crash.txt with a dev.");
    }
}
