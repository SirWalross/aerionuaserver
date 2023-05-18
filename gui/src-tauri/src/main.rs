use log::{debug, info, warn};
use std::{
    io::{Read, Write},
    net::{SocketAddr, TcpStream},
    path::PathBuf,
};
use tauri::{
    api::path::{config_dir, home_dir},
    Manager,
};

fn get_clients_file() -> PathBuf {
    if cfg!(windows) {
        config_dir()
            .unwrap()
            .join("Aerion OPC-UA Server")
            .join("clients.json")
    } else {
        home_dir()
            .unwrap()
            .join(".aerionuaserver")
            .join("clients.json")
    }
}

fn get_server_file() -> PathBuf {
    if cfg!(windows) {
        config_dir()
            .unwrap()
            .join("Aerion OPC-UA Server")
            .join("server.json")
    } else {
        home_dir()
            .unwrap()
            .join(".aerionuaserver")
            .join("server.json")
    }
}

fn get_log_dir() -> PathBuf {
    if cfg!(windows) {
        config_dir().unwrap().join("Aerion OPC-UA Server")
    } else {
        home_dir().unwrap().join(".aerionuaserver")
    }
}

fn convert_interface(interface: &ifcfg::IfCfg) -> String {
    let ipv4_address = interface
        .addresses
        .iter()
        .find(|address| matches!(address.address_family, ifcfg::AddressFamily::IPv4))
        .unwrap_or_else(|| interface.addresses.first().unwrap());
    let ipv6_address = interface
        .addresses
        .iter()
        .find(|address| matches!(address.address_family, ifcfg::AddressFamily::IPv6))
        .unwrap_or_else(|| interface.addresses.first().unwrap());
    format!("{{\"name\": \"{}\", \"mac-addr\": \"{}\", \"ipv4-addr\": {{\"address\": \"{}\", \"netmask\": \"{}\"}}, \"ipv6-addr\": {{\"address\": \"{}\", \"netmask\": \"{}\"}}}}",
        interface.name,
        interface.mac,
        ipv4_address.address.unwrap(),
        ipv4_address.mask.unwrap(),
        ipv6_address.address.unwrap(),
        ipv4_address.mask.unwrap(),
    )
}

#[tauri::command]
fn get_network_interfaces() -> Option<String> {
    let interfaces = ifcfg::IfCfg::get();
    if interfaces.is_err() {
        return None;
    }
    debug!("Gettings network interfaces");
    let interface_json: Vec<String> = interfaces
        .unwrap()
        .iter()
        .filter(|interface| {
            !interface.addresses.is_empty()
                && interface
                    .addresses
                    .iter()
                    .any(|address| !matches!(address.address_family, ifcfg::AddressFamily::Packet))
        })
        .map(convert_interface)
        .collect();

    Some(format!("[{}]", interface_json.join(", ")))
}

#[tauri::command]
fn get_devices() -> Option<String> {
    // read all devices from file and return as json list
    debug!("Reading devices");
    Some(std::fs::read_to_string(get_clients_file()).unwrap())
}

#[tauri::command]
fn write_to_server_file(key: String, value: String, value_type: String) -> Option<String> {
    // write a key value pair to server.json
    debug!("Write \"{}\": \"{}\" to server.json", key, value);
    let mut document: serde_json::Value =
        serde_json::from_str(&std::fs::read_to_string(get_server_file()).unwrap()).unwrap();

    let value = if value_type == "Number" {
        serde_json::Value::Number(value.parse().unwrap())
    } else if value_type == "Boolean" {
        serde_json::Value::Bool(value.parse().unwrap())
    } else if value_type == "Array" {
        serde_json::Value::Array(serde_json::from_str(&value).unwrap())
    } else {
        serde_json::Value::String(value)
    };

    if !document.as_object().unwrap().contains_key(&key) {
        // create new key value pair
        document.as_object_mut().unwrap().insert(key, value);
    } else {
        // update value
        document.as_object_mut().unwrap()[&key] = value;
    }

    std::fs::write(
        get_server_file(),
        serde_json::to_string_pretty(&document).unwrap(),
    )
    .unwrap();
    None
}

#[tauri::command]
fn read_from_server_file(key: String, value_type: String) -> Option<String> {
    // read value of key from server.json
    let document: serde_json::Value =
        serde_json::from_str(&std::fs::read_to_string(get_server_file()).unwrap()).unwrap();

    if document.as_object().unwrap().contains_key(&key) {
        let value = if value_type == "Number" {
            document.as_object().unwrap()[&key]
                .as_i64()
                .unwrap()
                .to_string()
        } else if value_type == "Array" {
            document.as_object().unwrap()[&key].to_string()
        } else {
            document.as_object().unwrap()[&key]
                .as_str()
                .unwrap()
                .to_string()
        };
        debug!("Reading \"{}\": \"{}\" from server.json", key, value);
        Some(value)
    } else {
        warn!(
            "Reading \"{}\" from server.json failed because key does not exist",
            key
        );
        None
    }
}

#[tauri::command]
fn add_device(device: String) -> Option<String> {
    // add a device, return error message if failed
    let mut document: serde_json::Value =
        serde_json::from_str(&std::fs::read_to_string(get_clients_file()).unwrap()).unwrap();

    let device: serde_json::Value = serde_json::from_str(device.as_str()).unwrap();
    let name = device["Name"].as_str().unwrap();

    // check for duplicates
    if document["Clients"]
        .as_array()
        .unwrap()
        .iter()
        .any(|value| value["Name"].as_str().unwrap() == name)
    {
        warn!(
            "Adding device '{}' failed because device is duplicate",
            device
        );
        return Some(format!("Duplicate device {}", name));
    } else if name == "running" {
        warn!("Adding device {} failed because name is invalid", device);
        return Some(format!("Invalid name {}", name));
    }

    debug!("Adding device '{}'", device);

    document["Clients"].as_array_mut().unwrap().push(device);

    std::fs::write(
        get_clients_file(),
        serde_json::to_string_pretty(&document).unwrap(),
    )
    .unwrap();
    None
}

#[tauri::command]
fn add_user_node(device_name: String, user_node: String) -> Option<String> {
    // add a user_node to a device, return error message if failed
    let mut document: serde_json::Value =
        serde_json::from_str(&std::fs::read_to_string(get_clients_file()).unwrap()).unwrap();

    let user_node: serde_json::Value = serde_json::from_str(user_node.as_str()).unwrap();
    let user_node_name = user_node["Name"].as_str().unwrap();
    let user_node_parent_name = user_node["Parent"].as_str().unwrap();

    // get device with `device_name`
    let device = document["Clients"]
        .as_array_mut()
        .unwrap()
        .iter_mut()
        .find(|device| device["Name"].as_str().unwrap() == device_name)
        .unwrap()
        .as_object_mut()
        .unwrap();

    // add user node
    if device.contains_key("UserNodes") {
        // check for duplicates
        if device["UserNodes"]
            .as_array()
            .unwrap()
            .iter()
            .any(|user_node| {
                user_node["Name"].as_str().unwrap() == user_node_name
                    && user_node["Parent"].as_str().unwrap() == user_node_parent_name
            })
        {
            warn!(
                "Adding user node '{}' to '{}' failed because user node is duplicate",
                device_name, user_node_name
            );
            return Some(format!("Duplicate user node '{}'", user_node_name));
        }
        debug!("Adding user node '{}'", user_node_name);

        device["UserNodes"].as_array_mut().unwrap().push(user_node);
    } else {
        // user nodes array not yet present
        debug!("Adding user node '{}'", user_node_name);
        device.insert(
            "UserNodes".to_string(),
            serde_json::Value::Array(vec![user_node]),
        );
    }

    std::fs::write(
        get_clients_file(),
        serde_json::to_string_pretty(&document).unwrap(),
    )
    .unwrap();
    None
}

#[tauri::command]
fn remove_user_node(device_name: String, user_node_name: String, user_node_parent: String) {
    // remove a user node
    let mut document: serde_json::Map<String, serde_json::Value> =
        serde_json::from_str(&std::fs::read_to_string(get_clients_file()).unwrap()).unwrap();

    // get device with `device_name`
    let user_nodes = document["Clients"]
        .as_array_mut()
        .unwrap()
        .iter_mut()
        .find(|device| device["Name"].as_str().unwrap() == device_name)
        .unwrap()
        .as_object_mut()
        .unwrap()["UserNodes"]
        .as_array_mut()
        .unwrap();

    let index = user_nodes
        .iter()
        .position(|user_node| {
            user_node["Name"].as_str().unwrap() == user_node_name
                && user_node["Parent"].as_str().unwrap() == user_node_parent
        })
        .unwrap();
    user_nodes.remove(index);

    debug!("Removing user node '{}'", user_node_name);

    std::fs::write(
        get_clients_file(),
        serde_json::to_string_pretty(&document).unwrap(),
    )
    .unwrap();
}

#[tauri::command]
fn remove_device(device: String) {
    // remove a device
    let mut document: serde_json::Map<String, serde_json::Value> =
        serde_json::from_str(&std::fs::read_to_string(get_clients_file()).unwrap()).unwrap();

    let devices: Vec<&serde_json::Value> = document
        .get_mut("Clients")
        .unwrap()
        .as_array_mut()
        .unwrap()
        .iter()
        .filter(|value| value["Name"].as_str().unwrap() != device)
        .collect();

    debug!("Removing device '{}'", device);

    std::fs::write(
        get_clients_file(),
        serde_json::to_string_pretty(&serde_json::json!({ "Clients": devices })).unwrap(),
    )
    .unwrap();
}

#[tauri::command]
fn check_connection(device: String) -> Option<String> {
    // check connection with device, returns error message if failed
    let device: serde_json::Value = serde_json::from_str(device.as_str()).unwrap();
    let address: SocketAddr = format!(
        "{}:{}",
        device["Ip"].as_str().unwrap(),
        device["Port"].as_i64().unwrap()
    )
    .parse()
    .expect("Unable to parse socket address");

    debug!(
        "Checking connection to {} '{}'",
        device["type"].as_str().unwrap(),
        device
    );

    let Ok(mut stream) = TcpStream::connect_timeout(&address, std::time::Duration::new(2, 0)) else {return Some("Couldnt connect to device".to_string())};

    match stream.set_write_timeout(Some(std::time::Duration::new(2, 0))) {
        Ok(_) => {}
        Err(_) => return Some("Couldn't set write timeout".to_string()),
    }

    match stream.set_read_timeout(Some(std::time::Duration::new(2, 0))) {
        Ok(_) => {}
        Err(_) => return Some("Couldn't set read timeout".to_string()),
    }

    if device["Type"].as_str().unwrap() == "Robot" {
        let Ok(_) = stream.write_all(b"1;1;OPEN=ROBOT") else {return Some("Couldn't send message device".to_string())};
        let mut answer = [0u8; 240];
        let Ok(_) = stream.read(&mut answer) else {return Some("Couldn't receive answer from device".to_string())};
        let Ok(received) = std::str::from_utf8(&answer) else {return Some("Invalid answer from device".to_string())};

        if !received.to_ascii_lowercase().starts_with("qok") {
            return Some("Invalid answer from device".to_string());
        }
    } else if device["Type"].as_str().unwrap() == "PLC" {
        // loopback test -> client should answer with same message
        let buf = [
            0x50u8, 0x00, 0x00, 0xff, 0xff, 0x03, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x19, 0x06, 0x00,
            0x00, 0x02, 0x00, 0x41, 0x42,
        ];
        let Ok(_) = stream.write_all(&buf) else {return Some("Couldn't send message device".to_string())};
        let mut answer = [0u8; 240];
        let Ok(len) = stream.read(&mut answer) else {return Some("Couldn't receive answer from device".to_string())};
        if len != 15 {
            return Some("Invalid answer from device".to_string());
        } else if answer[9] as u32 + ((answer[10] as u32) << 8) != 0 {
            return Some("Endcode != 0".to_string());
        } else if answer[11] as u32 + ((answer[12] as u32) << 8) != 2 {
            return Some("Loopback data length invalid".to_string());
        } else if answer[13] != buf[17] || answer[14] != buf[18] {
            return Some("Loopback data invalid".to_string());
        }
    } else {
        panic!("Invalid device type");
    }

    match stream.shutdown(std::net::Shutdown::Both) {
        Ok(_) => None,
        Err(_) => Some("Couldn't shutdown device".to_string()),
    }
}

#[derive(Clone, serde::Serialize)]
struct Payload {
    message: String,
}

fn main() {
    tauri::Builder::default()
        .setup(|app| {
            info!("Starting gui");
            let window = app.get_window("main").unwrap();

            app.once_global("startup", move |_| {
                let ctx = zmq::Context::new();
                let socket = ctx.socket(zmq::REP).unwrap();
                socket.connect("tcp://localhost:5555").unwrap();

                let mut msg = zmq::Message::new();

                loop {
                    socket.recv(&mut msg, 0).unwrap();
                    if msg.as_str().unwrap().starts_with("{\"device_update\"") {
                        window
                            .emit(
                                "device_update",
                                Payload {
                                    message: msg.as_str().unwrap().to_string(),
                                },
                            )
                            .unwrap();
                    } else if msg.as_str().unwrap().starts_with("{\"server_update\"") {
                        window
                            .emit(
                                "server_update",
                                Payload {
                                    message: msg.as_str().unwrap().to_string(),
                                },
                            )
                            .unwrap();
                    } else if msg.as_str().unwrap().starts_with("{\"clear_devices\"") {
                        window
                            .emit(
                                "clear_devices",
                                Payload {
                                    message: msg.as_str().unwrap().to_string(),
                                },
                            )
                            .unwrap();
                    }
                    socket.send("OK", 0).unwrap();
                }
            });

            Ok(())
        })
        .plugin(tauri_plugin_fs_extra::init())
        .plugin(tauri_plugin_fs_watch::init())
        .plugin(
            tauri_plugin_log::Builder::default()
                .target(tauri_plugin_log::LogTarget::Folder(get_log_dir()))
                .level(log::LevelFilter::Info)
                .max_file_size(1048576)
                .build(),
        )
        .invoke_handler(tauri::generate_handler![
            get_network_interfaces,
            get_devices,
            add_device,
            remove_device,
            check_connection,
            add_user_node,
            remove_user_node,
            write_to_server_file,
            read_from_server_file
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
