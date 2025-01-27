use std::{error::Error, sync::{atomic::{AtomicBool, Ordering}, Arc}, time::Duration};

use dbus::{blocking::Connection, message::MatchRule};


const DBUS_PROCESS_TIMEOUT: Duration = Duration::from_millis(100);


pub struct SleepMonitor {
    is_sleeping: Arc<AtomicBool>,
    connection: Connection,
}

impl SleepMonitor {
    pub fn new() -> Result<Self, Box<dyn Error>> {
        let conn = Connection::new_system()?;
        let is_sleeping = Arc::new(AtomicBool::new(false));
        
        Ok(SleepMonitor {
            is_sleeping,
            connection: conn,
        })
    }

    pub fn start_monitoring(&self) -> Result<(), Box<dyn Error>> {
        let mut rule = MatchRule::new();
        rule.interface = Some("org.freedesktop.login1.Manager".into());
        rule.member = Some("PrepareForSleep".into());
        
        let is_sleeping = self.is_sleeping.clone();
        
        self.connection.add_match(rule, move |_: (), _, msg| {
            if let Some(sleeping) = msg.get1::<bool>() {
                is_sleeping.store(sleeping, Ordering::SeqCst);
                if sleeping {
                    println!("System is preparing to sleep");
                } else {
                    println!("System is resuming from sleep");
                }
            }
            true
        })?;

        Ok(())
    }

    pub fn is_sleeping(&self) -> bool {
        self.is_sleeping.load(Ordering::SeqCst)
    }

    pub fn get_sleep_state(&self) -> Arc<AtomicBool> {
        self.is_sleeping.clone()
    }

    pub fn process_events(&self) -> Result<(), Box<dyn Error>> {
        self.connection.process(DBUS_PROCESS_TIMEOUT)?;
        Ok(())
    }
}

