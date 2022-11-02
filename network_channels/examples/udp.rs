use instant::Duration;
use network_channels::{deserialize, Config, ConnectionBuilder, Instant, Reliability};
use std::net::UdpSocket;

fn main() {
    let socket_a = UdpSocket::bind("0.0.0.0:0").unwrap();
    let socket_b = UdpSocket::bind("0.0.0.0:0").unwrap();
    let addr_b = socket_b.local_addr().unwrap();

    let config = Config::default();

    // A is the sender
    let mut builder_a = ConnectionBuilder::new(&config);
    let channel_a = builder_a.sending_channel(());
    let (mut a_sender, _) = builder_a.build().split();

    channel_a.send_serialized(Reliability::Reliable, "Hello!");

    // B is only listening on the other side
    let mut builder_b = ConnectionBuilder::new(&config);
    builder_b.receiving_channel(()); // a channel with a matching label must exist to listen
    let (_, mut b_receiver) = builder_b.build().split();

    // listen loop
    std::thread::spawn(move || {
        // same as config.mtu
        let mut buf = [0u8; 1400];

        while let Ok(len) = socket_b.recv(&mut buf) {
            let now = Instant::now();

            match b_receiver.receive_packet(now, &buf[0..len]) {
                Ok(Some((_channel, messages))) => {
                    for message in messages {
                        let message: &str = deserialize(&message).unwrap();
                        println!("{message}");
                    }
                }
                Ok(None) => {
                    // occurs if the packet is for internal use or there's no channel configured to listen
                }
                Err(e) => println!("{e}"),
            }
        }
    });

    // send loop
    loop {
        let now = Instant::now();

        a_sender.tick(now, |bytes| {
            let _ = socket_a.send_to(bytes, addr_b);
        });

        std::thread::sleep(Duration::from_millis(50));
    }
}
