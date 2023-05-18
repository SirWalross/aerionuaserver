import { invoke } from "@tauri-apps/api/tauri";

export async function load() {
    const port: string = await invoke('read_from_server_file', {key: "Port", valueType: "Number"}) ?? "4840";
    const security_policies: string = await invoke('read_from_server_file', { key: "SecurityPolicies", valueType: "Array" }) ?? "[\"None\"]";
    const security_modes: string = await invoke('read_from_server_file', { key: "SecurityModes", valueType: "Array" }) ?? "[]";
    const enforce_client_certificate_verification: number[] = (await invoke('read_from_server_file', { key: "EnforceClientCertificateVerification", valueType: "Array" }) !== null) ? [1] : [];
    return {
        port: parseInt(port),
        security_policies: JSON.parse(security_policies),
        security_modes: JSON.parse(security_modes),
        enforce_client_certificate_verification: enforce_client_certificate_verification
    };
}