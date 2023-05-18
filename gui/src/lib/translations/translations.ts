import i18n from 'sveltekit-i18n';
import lang from './lang.json';

/** @type {import('sveltekit-i18n').Config} */
const config = ({
    translations: {
        en: { lang },
        ja: { lang },
      },
    loaders: [
        {
            locale: 'en',
            key: 'common',
            loader: async () => (
                await import('./en/common.json')
            ).default,
        },
        {
            locale: 'en',
            key: 'home',
            routes: ['/'],
            loader: async () => (
                await import('./en/home.json')
            ).default,
        },
        {
            locale: 'en',
            key: 'devices',
            routes: ['/devices'],
            loader: async () => (
                await import('./en/devices.json')
            ).default,
        },
        {
            locale: 'en',
            key: 'about',
            routes: ['/about'],
            loader: async () => (
                await import('./en/about.json')
            ).default,
        },
        {
            locale: 'en',
            key: 'logs',
            routes: ['/logs'],
            loader: async () => (
                await import('./en/logs.json')
            ).default,
        },
        {
            locale: 'en',
            key: 'interfaces',
            routes: ['/interfaces'],
            loader: async () => (
                await import('./en/interfaces.json')
            ).default,
        },
        {
            locale: 'en',
            key: 'settings',
            routes: ['/settings'],
            loader: async () => (
                await import('./en/settings.json')
            ).default,
        },
        {
            locale: 'ja',
            key: 'common',
            loader: async () => (
                await import('./ja/common.json')
            ).default,
        },
        {
            locale: 'ja',
            key: 'home',
            routes: ['/'],
            loader: async () => (
                await import('./ja/home.json')
            ).default,
        },
        {
            locale: 'ja',
            key: 'devices',
            routes: ['/devices'],
            loader: async () => (
                await import('./ja/devices.json')
            ).default,
        },
        {
            locale: 'ja',
            key: 'about',
            routes: ['/about'],
            loader: async () => (
                await import('./ja/about.json')
            ).default,
        },
        {
            locale: 'ja',
            key: 'logs',
            routes: ['/logs'],
            loader: async () => (
                await import('./ja/logs.json')
            ).default,
        },
        {
            locale: 'ja',
            key: 'interfaces',
            routes: ['/interfaces'],
            loader: async () => (
                await import('./ja/interfaces.json')
            ).default,
        },
        {
            locale: 'ja',
            key: 'settings',
            routes: ['/settings'],
            loader: async () => (
                await import('./ja/settings.json')
            ).default,
        },
    ],
});

export const { t, locale, locales, loading, loadTranslations } = new i18n(config);