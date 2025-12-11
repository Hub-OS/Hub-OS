use network_channels::{
    ChannelSender, Config, ConnectionBuilder, Instant, PacketReceiver, PacketSender, Reliability,
    deserialize,
};
use rand::{SeedableRng, rngs, seq::SliceRandom};

fn create_config() -> Config {
    Config {
        // send at max speed
        initial_bytes_per_second: usize::MAX,
        max_bytes_per_second: Some(usize::MAX),
        // don't speed up
        bytes_per_second_increase_factor: 1.0,
        // don't slow down
        bytes_per_second_slow_factor: 1.0,
        // retry immediately
        rtt_resend_factor: 0.0,
        ..Config::default()
    }
}

fn create_connection(config: &Config) -> (ChannelSender<()>, PacketSender<()>, PacketReceiver<()>) {
    let mut builder = ConnectionBuilder::new(config);
    let channel = builder.bidirectional_channel(());
    let (sender, receiver) = builder.build().split();
    (channel, sender, receiver)
}

fn start_test_environment(
    send: &mut dyn FnMut(&ChannelSender<()>),
    receive: &mut dyn FnMut(Vec<Vec<u8>>),
) {
    const RUNS: usize = 100;
    const MESSAGES_PER_RUN: usize = 100;
    const DROPPED_PER_RUN: usize = 5;

    let config = create_config();
    // A is the sender
    let (channel_a, mut sender_a, mut receiver_a) = create_connection(&config);
    // B is listening and sending acks on the other side
    let (_, mut sender_b, mut receiver_b) = create_connection(&config);

    let mut rng = rngs::SmallRng::from_os_rng();
    let mut sent_messages = vec![vec![]; MESSAGES_PER_RUN];

    for _ in 0..RUNS {
        // queue messages to send
        for _ in 0..MESSAGES_PER_RUN {
            send(&channel_a);
        }

        // simulate communication, stop when every message has been received
        let mut received = 0;
        let mut pending_drop = DROPPED_PER_RUN;

        while received != MESSAGES_PER_RUN {
            let now = Instant::now();

            // clear buckets
            for message in &mut sent_messages {
                message.clear();
            }

            // send messages
            let mut sent = 0;

            sender_a.tick(now, |bytes| {
                let Some(bucket) = sent_messages.get_mut(sent) else {
                    return;
                };

                bucket.extend_from_slice(bytes);
                sent += 1;
            });

            // shuffle messages
            sent_messages[..sent].shuffle(&mut rng);

            // read them
            for message in &sent_messages[..sent.saturating_sub(pending_drop)] {
                let Some((_channel, read_messages)) =
                    receiver_b.receive_packet(now, message).unwrap()
                else {
                    continue;
                };

                received += read_messages.len();
                receive(read_messages);
            }

            // already dropped messages, allow the resends through
            pending_drop = 0;

            // handle acks
            sender_b.tick(now, |message| {
                let _ = receiver_a.receive_packet(now, message);
            });
        }
    }
}

#[test]
fn reliable_ordered() {
    let mut next_send_value = 1;
    let mut last_read_value = 0;

    start_test_environment(
        &mut |channel| {
            channel.send_serialized(Reliability::ReliableOrdered, next_send_value);
            next_send_value += 1;
        },
        &mut |messages| {
            for message in messages {
                let read_value = deserialize(&message).unwrap();

                assert_eq!(
                    last_read_value + 1,
                    read_value,
                    "must arrive in the correct order"
                );

                last_read_value = read_value;
            }
        },
    );

    assert_eq!(
        last_read_value + 1,
        next_send_value,
        "must receive all messages"
    );
}

#[test]
fn reliable() {
    let mut next_send_value: i32 = 1;
    let mut next_read_value: i32 = 1;

    let mut messages_read = 0;
    let mut unresolved_messages = Vec::new();

    start_test_environment(
        &mut |channel| {
            channel.send_serialized(Reliability::Reliable, next_send_value);
            next_send_value += 1;
        },
        &mut |messages| {
            for message in messages {
                let read_value: i32 = deserialize(&message).unwrap();

                unresolved_messages.push(read_value);

                messages_read += 1;
            }

            unresolved_messages.sort_by(|a, b| b.cmp(a));

            while unresolved_messages.last() == Some(&next_read_value) {
                unresolved_messages.pop();
                next_read_value += 1;
            }
        },
    );

    assert_eq!(unresolved_messages.len(), 0, "must receive all messages");

    assert_eq!(
        messages_read + 1,
        next_send_value,
        "must receive all messages"
    );
}
