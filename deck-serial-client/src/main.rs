mod sleep_monitor;

use std::{error::Error, io::{self, Write}, thread, time::Duration};

use serialport::{SerialPort, SerialPortInfo, SerialPortType};
use sleep_monitor::SleepMonitor;

/**
 * The baud rate to use when communicating to the Arduino over USB
 */
const BAUD_RATE: u32 = 9600;
/**
 * The time between heartbeat messages
 */
const INTERVAL: Duration = Duration::from_secs(5);
/**
 * Time between retrying the serial connection
 */
const DELAY: Duration = Duration::from_secs(8);


fn find_arduino_port(ports: &Vec<SerialPortInfo>) -> Option<&SerialPortInfo> {
    for port in ports {
        match &port.port_type {
            SerialPortType::UsbPort(usb_info) => {
                let manufacturer = usb_info.manufacturer.clone().unwrap_or("".into());
                let product = usb_info.product.clone().unwrap_or("".into());
                if manufacturer == "Arduino LLC" && product.contains("Arduino") {
                    return Some(port);
                }
            },
            _ => (),
        }
    }

    None
}

fn heartbeat_loop(mut arduino: Box<dyn SerialPort>, monitor: &SleepMonitor) {
    loop {
        monitor.process_events().expect("Failed to process DBus events");
        if monitor.is_sleeping() {
            thread::sleep(INTERVAL);
            continue;
        }

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
    }
}

fn main() -> Result<(), Box<dyn Error>> {
    let monitor = SleepMonitor::new()?;
    monitor.start_monitoring()?;

    loop {
        // Find any Serial ports
        let ports = match serialport::available_ports() {
            Ok(ports) => ports,
            Err(_) => vec![],
        };

        for port in &ports {
            println!("{}:", port.port_name);
            println!("\tport_type: {:?}", port.port_type);

            match &port.port_type {
                SerialPortType::UsbPort(usb_info) => println!("\t\tUSB Port Info: {:?}", usb_info),
                _ => (),
            }
        }

        match find_arduino_port(&ports) {
            Some(arduino_info) => {
                let arduino_builder = serialport::new(arduino_info.port_name.clone(), BAUD_RATE)
                    // Settings found using Arduino IDE's built-in Serial Monitor.
                    .stop_bits(serialport::StopBits::One)
                    .data_bits(serialport::DataBits::Eight)
                    .parity(serialport::Parity::None)
                    .dtr_on_open(true);

                match arduino_builder.open() {
                    Ok(arduino) => heartbeat_loop(arduino, &monitor),
                    Err(e) => eprintln!("Failed to connect to Arduino: {:?}", e),
                }
            },
            None => eprintln!("No Arduino found"),
        }

        // Connection closed, wait before attempting to reconnect
        thread::sleep(DELAY);
    }
}
