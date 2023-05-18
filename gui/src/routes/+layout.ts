import { loadTranslations, locale } from '$lib/translations/translations';
import { invoke } from '@tauri-apps/api/tauri';
import type { LayoutLoad } from './$types';

export const load: LayoutLoad = async ({ url }) => {
    const { pathname } = url;
    const defaultLocale = await invoke('read_from_server_file', {key: "Language", valueType: "String"}) ?? "en";
    const initLocale = locale.get() || defaultLocale; // set default if no locale already set
    await loadTranslations(initLocale, pathname); // keep this just before the `return`

    return {};
}

export const prerender = true;
export const ssr = false;