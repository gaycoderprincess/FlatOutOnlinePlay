use std::collections::HashMap;
use std::net::SocketAddr;
use std::sync::OnceLock;
use std::sync::{Arc, Mutex};
use std::time::Duration;

use axum::extract::{ConnectInfo, Path, State};
use axum::routing::{get, post};
use axum::Router;
use serde::Deserialize;
use tokio::time;
use tokio_stream::wrappers::IntervalStream;
use tokio_stream::StreamExt;

#[derive(Deserialize)]
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

struct ServerInfo {
    lobby_port: u16,
    game_port: u16,
    time_left: u8,
}

static TIME: OnceLock<u8> = OnceLock::new();

#[tokio::main]
async fn main() {
    let config: Config = xnya_utils::read_toml("fo1-master-server.toml")
        .unwrap()
        .unwrap_or_default();

    TIME.get_or_init(|| config.timeout_secs);

    let server_info: HashMap<String, ServerInfo> = HashMap::new();
    let server_info = Arc::new(Mutex::new(server_info));

    let cloned_info = server_info.clone();

    tokio::spawn(async move {
        let mut stream = IntervalStream::new(time::interval(Duration::from_secs(1)));

        while stream.next().await.is_some() {
            tick(cloned_info.clone()).await;
        }
    });

    let app = Router::new()
        .route(
            "/get-lobbies/:lobby_port/:game_port",
            get(get_lobbies).with_state(server_info.clone()),
        )
        .route(
            "/add-lobby/:lobby_port/:game_port",
            post(add_lobby).with_state(server_info),
        );
    let listener = tokio::net::TcpListener::bind(config.listen_ip)
        .await
        .unwrap();
    axum::serve(
        listener,
        app.into_make_service_with_connect_info::<SocketAddr>(),
    )
    .await
    .unwrap();
}

async fn add_lobby(
    ConnectInfo(addr): ConnectInfo<SocketAddr>,
    Path((lobby_port, game_port)): Path<(u16, u16)>,
    State(server_info): State<Arc<Mutex<HashMap<String, ServerInfo>>>>,
) {
    let mut server_info = server_info.lock().unwrap();

    server_info.insert(
        addr.ip().to_string(),
        ServerInfo {
            lobby_port,
            game_port,
            time_left: *TIME.get().unwrap(),
        },
    );
}

async fn get_lobbies(
    Path((lobby_port, game_port)): Path<(u16, u16)>,
    State(server_info): State<Arc<Mutex<HashMap<String, ServerInfo>>>>,
) -> String {
    server_info
        .lock()
        .unwrap()
        .iter()
        .filter_map(|(ip, info)| {
            if info.lobby_port == lobby_port && info.game_port == game_port {
                Some(ip)
            } else {
                None
            }
        })
        .cloned()
        .collect::<Vec<String>>()
        .join("\n")
}

async fn tick(server_info: Arc<Mutex<HashMap<String, ServerInfo>>>) {
    let mut server_info = server_info.lock().unwrap();

    server_info.iter_mut().for_each(|(_, info)| {
        info.time_left -= 1;
    });

    server_info.retain(|_, info| info.time_left != 0);
}
