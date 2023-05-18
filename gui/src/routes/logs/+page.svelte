<script>
	import { Tabs, TabItem } from 'flowbite-svelte';
	import Log from '$lib/Log.svelte';
	import { join, homeDir, configDir } from '@tauri-apps/api/path';
	import { type } from '@tauri-apps/api/os';
	import { t } from '$lib/translations/translations';
</script>

<Tabs
	contentClass="mt-4 mb-4"
	activeClasses="p-4 text-blue-600 bg-gray-200 rounded-t-lg dark:bg-gray-800 dark:text-blue-500"
>
	{#await type() then _type}
		<TabItem open contentClass="">
			<span slot="title">{$t('logs.server logs')}</span>
			{#if _type == 'Linux'}
				{#await homeDir() then home_dir}
					{#await join(home_dir, '.aerionuaserver', 'server.log') then filepath}
						<Log {filepath} defaultFilename="server.log" />
					{/await}
				{/await}
			{:else}
				{#await configDir() then config_dir}
					{#await join(config_dir, 'Aerion OPC-UA Server', 'server.log') then filepath}
						<Log {filepath} defaultFilename="server.log" />
					{/await}
				{/await}
			{/if}
		</TabItem>
		<TabItem contentClass="">
			<span slot="title">{$t('logs.gui logs')}</span>
			{#if _type == 'Linux'}
				{#await homeDir() then home_dir}
					{#await join(home_dir, '.aerionuaserver', 'aerionuaserver-gui.log') then filepath}
						<Log {filepath} defaultFilename="gui.log" />
					{/await}
				{/await}
			{:else}
				{#await configDir() then config_dir}
					{#await join(config_dir, 'Aerion OPC-UA Server', 'aerionuaserver-gui.log') then filepath}
						<Log {filepath} defaultFilename="gui.log" />
					{/await}
				{/await}
			{/if}
		</TabItem>
	{/await}
</Tabs>
