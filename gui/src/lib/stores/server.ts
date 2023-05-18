import { listen } from '@tauri-apps/api/event'
import { readable } from 'svelte/store';

export const server = readable(false, set => {
    let value = {};
    listen('server_update', (event) => {
        value = Object.assign(value, JSON.parse(event.payload.message)["server_update"]);
        set(value);
    });
});
