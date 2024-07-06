use documented::DocumentedFields;
use serde::Serialize;
use std::env;
use std::error::Error;

use xnya_utils::DocumentedStruct;
impl DocumentedStruct for Config {
    fn get_doc_string(location: &str, field: &str) -> Option<&'static str> {
        match location {
            "" => Config::get_field_docs(field).ok(),
            _ => None,
        }
    }
}
#[derive(Serialize, DocumentedFields)]
struct Config {
    /// The IP and port the master server should listen on
    listen_ip: String,
    /// Number of seconds to hold a server for, 0-255
    timeout_secs: u8,
}

impl Default for Config {
    fn default() -> Self {
        Self {
            listen_ip: "0.0.0.0:23755".to_string(),
            timeout_secs: 15,
        }
    }
}

fn main() -> Result<(), Box<dyn Error>> {
    xnya_utils::write_toml_comments(
        &format!(
            "{}/target/fo1-master-server.toml",
            env::var("CARGO_MANIFEST_DIR").unwrap()
        ),
        &Config::default(),
    )?;

    Ok(())
}
