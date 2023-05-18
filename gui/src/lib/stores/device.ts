import { listen } from '@tauri-apps/api/event'
import { readable } from 'svelte/store';

export const devices = readable(false, set => {
    let value = {};
    listen('device_update', (event) => {
        value = Object.assign(value, JSON.parse(event.payload.message)["device_update"]);
        set(value);
    });
    listen('clear_devices', (_) => {
        set({});
    })
});
