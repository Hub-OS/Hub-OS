// does not test packet reliability

use network_channels::{
    ChannelSender, Config, ConnectionBuilder, Instant, PacketReceiver, PacketSender, Reliability,
    deserialize,
};
use rand::{SeedableRng, rngs, seq::SliceRandom};

fn build_connection() -> (ChannelSender<()>, PacketSender<()>, PacketReceiver<()>) {
    let config = Config {
        // don't slow down
        bytes_per_second_slow_factor: 1.0,
        initial_bytes_per_second: usize::MAX,
        max_bytes_per_second: Some(usize::MAX),
        ..Config::default()
    };

    // A is the sender
    let mut builder_a = ConnectionBuilder::new(&config);
    let channel_a = builder_a.sending_channel(());
    let (a_sender, _) = builder_a.build().split();

    // B is only listening on the other side
    let mut builder_b = ConnectionBuilder::new(&config);
    builder_b.receiving_channel(()); // a channel with a matching label must exist to listen
    let (_, b_receiver) = builder_b.build().split();

    (channel_a, a_sender, b_receiver)
}

#[test]
fn reliable_ordered() {
    let (channel_a, mut a_sender, mut b_receiver) = build_connection();

    let mut rng = rngs::SmallRng::from_os_rng();
    let mut sent_messages = vec![vec![]; 100];
    let mut next_send_value = 1;
    let mut last_read_value = 0;

    for _ in 0..100 {
        for _ in 0..sent_messages.len() {
            channel_a.send_serialized(Reliability::ReliableOrdered, next_send_value);
            next_send_value += 1;
        }

        // send messages to our messages vec
        let now = Instant::now();
        let mut i = 0;

        a_sender.tick(now, |bytes| {
            let bucket = &mut sent_messages[i];
            i += 1;

            bucket.clear();
            bucket.extend_from_slice(bytes);
        });

        // shuffle messages
        sent_messages.shuffle(&mut rng);

        // read them
        for message in &sent_messages {
            let (_channel, read_messages) =
                b_receiver.receive_packet(now, message).unwrap().unwrap();

            for message in read_messages {
                let read_value = deserialize(&message).unwrap();

                assert_eq!(
                    last_read_value + 1,
                    read_value,
                    "must arrive in the correct order"
                );

                last_read_value = read_value;
            }
        }
    }

    assert_eq!(
        last_read_value + 1,
        next_send_value,
        "must receive all messages"
    );
}

#[test]
fn reliable() {
    let (channel_a, mut a_sender, mut b_receiver) = build_connection();

    let mut rng = rngs::SmallRng::from_os_rng();
    let mut sent_messages = vec![vec![]; 100];
    let mut next_send_value: i32 = 1;
    let mut next_read_value: i32 = 1;

    let mut messages_read = 0;
    let mut unresolved_messages = Vec::new();

    for _ in 0..100 {
        for _ in 0..sent_messages.len() {
            channel_a.send_serialized(Reliability::Reliable, next_send_value);
            next_send_value += 1;
        }

        // send messages to our messages vec
        let now = Instant::now();
        let mut i = 0;

        a_sender.tick(now, |bytes| {
            let bucket = &mut sent_messages[i];
            i += 1;

            bucket.clear();
            bucket.extend_from_slice(bytes);
        });

        // shuffle messages
        sent_messages.shuffle(&mut rng);

        // read them
        for message in &sent_messages {
            let (_channel, read_messages) =
                b_receiver.receive_packet(now, message).unwrap().unwrap();

            for message in read_messages {
                let read_value: i32 = deserialize(&message).unwrap();

                unresolved_messages.push(read_value);

                messages_read += 1;
            }

            unresolved_messages.sort_by(|a, b| b.cmp(a));

            while unresolved_messages.last() == Some(&next_read_value) {
                unresolved_messages.pop();
                next_read_value += 1;
            }
        }
    }

    assert!(unresolved_messages.is_empty(), "must receive all messages");

    assert_eq!(
        messages_read + 1,
        next_send_value,
        "must receive all messages"
    );
}
