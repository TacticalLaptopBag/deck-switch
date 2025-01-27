use std::{io, io::Write, thread, time::Duration};

/**
 * The baud rate to use when communicating to the Arduino over USB
 */
const BAUD_RATE: u32 = 9600;
/**
 * The time between heartbeat messages
 */
const INTERVAL: Duration = Duration::from_secs(1);
/**
 * Time between retrying the serial connection
 */
const DELAY: Duration = Duration::from_secs(8);


fn main() {
    loop {
        // Find any Serial ports
        let ports = match serialport::available_ports() {
            Ok(ports) => ports,
            Err(_) => vec![],
        };

        if !ports.is_empty() {
            // Assume that the first port is an Arduino
            // TODO: This should be verified before blindly connecting to it
            let arduino_info = &ports[0];
            let arduino_builder = serialport::new(arduino_info.port_name.clone(), BAUD_RATE)
                // Settings found using Arduino IDE's built-in Serial Monitor.
                .stop_bits(serialport::StopBits::One)
                .data_bits(serialport::DataBits::Eight)
                .parity(serialport::Parity::None)
                .dtr_on_open(true);

            match arduino_builder.open() {
                Ok(mut arduino) => loop {
                    // Write the heartbeat byte to the port
                    match arduino.write("!".as_bytes()) {
                        Ok(_) => println!("!"),
                        Err(ref e) if e.kind() == io::ErrorKind::BrokenPipe => {
                            // Connection closed, break out of this loop to attempt to reconnect
                            eprintln!("Broken pipe");
                            break;
                        }
                        Err(ref e) if e.kind() == io::ErrorKind::TimedOut => eprintln!("Write timed out"),
                        Err(e) => eprintln!("{:?}", e),
                    }

                    // Wait before writing the next heartbeat
                    thread::sleep(INTERVAL);
                },
                Err(e) => eprintln!("Failed to connect to Arduino: {:?}", e),
            }
        }

        // Connection closed, wait before attempting to reconnect
        thread::sleep(DELAY);
    }
}
