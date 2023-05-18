<script lang="ts">
	import { Heading, P, A } from 'flowbite-svelte';
	import { server } from '$lib/stores/server';
	import { devices } from '$lib/stores/device';
	import { emit } from '@tauri-apps/api/event';
	import { t } from '$lib/translations/translations';

	let server_running: boolean;
	let device_count: number;
	let server_address: string;

	server.subscribe((value) => {
		server_running = value?.['running'] ?? false;
		server_address = value?.['address'] ?? '';
	});

	devices.subscribe((value) => {
		device_count = Object.values(value)?.filter((value) => value).length;
	});

	emit('startup');

	document.addEventListener('contextmenu', (event) => event.preventDefault());
</script>

<div class="text-center mt-10">
	<Heading tag="h1" class="mb-4" customSize="text-4xl font-extrabold  md:text-5xl lg:text-6xl"
		>Aerion OPC-UA Server</Heading
	>
	{#if server_running}
		<P class="mb-6 text-lg lg:text-xl sm:px-16 xl:px-48 dark:text-gray-400 text-center"
			>{$t('home.server running.prefix')}<A class="font-medium" disabled>{server_address}</A>{$t(
				'home.server running.suffix'
			)}</P
		>

		<P class="mb-6 text-lg lg:text-xl sm:px-16 xl:px-48 dark:text-gray-400 text-center"
			>{$t('home.connected.prefix')}{device_count}{$t('home.connected.suffix')}</P
		>
	{:else}
		<P class="mb-6 text-lg lg:text-xl sm:px-16 xl:px-48 dark:text-gray-400 text-center"
			>{$t('home.server not running')}</P
		>
	{/if}
</div>
