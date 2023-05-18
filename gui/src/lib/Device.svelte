<script lang="ts">
	import {
		Card,
		Indicator,
		Button,
		ButtonGroup,
		Checkbox,
		Tooltip,
		Spinner
	} from 'flowbite-svelte';
	import { createEventDispatcher } from 'svelte';
	import { devices } from '$lib/stores/device';
	import { server } from '$lib/stores/server';
	import { t } from '$lib/translations/translations';
	import { invoke } from '@tauri-apps/api/tauri';
	import { open } from '@tauri-apps/api/dialog';
	import { readBinaryFile } from '@tauri-apps/api/fs';
	import { info, warn } from 'tauri-plugin-log-api';
	import {
		plc_datatype,
		plc_device_type,
		plc_module_io,
		plc_variable_type,
		robot_datatype
	} from './forms/selectdata';

	const dispatch = createEventDispatcher();

	export let device;
	export let editing: boolean;
	export let value: number;
	export let user_nodes: any;
	export let device_type: string;

	let show_more: boolean = false;

	let server_running: boolean;
	let device_running: boolean;

	devices.subscribe((value) => {
		device_running = value?.[device['Name']] ?? null;
	});

	server.subscribe((value) => {
		server_running = value?.['running'] ?? false;
	});
</script>

<div class="flex">
	{#if editing}
		<Checkbox
			class="m-3 mr-5"
			{value}
			on:change={(e) => {
				dispatch('change', { index: value });
			}}
		/>
	{:else if server_running && device_running == null}
		{#if device_running}
			<Indicator
				size="xl"
				color="none"
				class="bg-green-400 dark:bg-green-700 z-10 flex-none m-2 mr-4 self-center"
			>
				<svg
					aria-hidden="true"
					class="w-4 h-4 text-gray-100 dark:text-gray-800"
					fill="currentColor"
					viewBox="0 0 20 20"
					xmlns="http://www.w3.org/2000/svg"
					><path
						fill-rule="evenodd"
						d="M16.707 5.293a1 1 0 010 1.414l-8 8a1 1 0 01-1.414 0l-4-4a1 1 0 011.414-1.414L8 12.586l7.293-7.293a1 1 0 011.414 0z"
						clip-rule="evenodd"
					/></svg
				>
			</Indicator>
			<Tooltip arrow={false}>{$t('devices.device running')}</Tooltip>
		{:else}
			<Indicator
				size="xl"
				color="none"
				class="bg-red-400 dark:bg-red-700 z-10 flex-none m-2 mr-4 self-center"
			>
				<svg
					class="w-4 h-4 text-gray-100 dark:text-gray-800"
					fill="currentColor"
					viewBox="0 0 20 20"
					xmlns="http://www.w3.org/2000/svg"
					><path
						fill-rule="evenodd"
						d="M4.293 4.293a1 1 0 011.414 0L10 8.586l4.293-4.293a1 1 0 111.414 1.414L11.414 10l4.293 4.293a1 1 0 01-1.414 1.414L10 11.414l-4.293 4.293a1 1 0 01-1.414-1.414L8.586 10 4.293 5.707a1 1 0 010-1.414z"
						clip-rule="evenodd"
					/></svg
				>
			</Indicator>
			<Tooltip arrow={false}>{$t('devices.device not running')}</Tooltip>
		{/if}
	{:else if !server_running}
		<Indicator
			id="server-not-running-indicator"
			size="xl"
			color="none"
			class="z-10 flex-none m-2 mr-4 bg-gray-300 dark:bg-gray-400 self-center"
		/>
		<Tooltip arrow={false}>{$t('devices.server not running')}</Tooltip>
	{:else}
		<Indicator
			id="device-not-running-indicator"
			size="xl"
			color="none"
			class="z-10 flex-none m-2 mr-4 self-center"
		>
			<Spinner size="6" />
		</Indicator>
		<Tooltip arrow={false}>{$t('devices.device starting')}</Tooltip>
	{/if}
	<Card
		padding="none"
		size="lg"
		class="p-2 mb-1 flex-1 w-full text-ellipsis overflow-hidden whitespace-nowrap"
	>
		<p class="text-sm font-medium text-gray-900 truncate dark:text-white">
			{device['Name']} · {device['Ip']}:{device['Port']}
		</p>
		{#if device_type === 'plc'}
			<p class="text-sm text-gray-500 truncate dark:text-gray-400">
				{plc_module_io[device['Destination Module I/O']]}
			</p>
		{/if}
	</Card>
	{#if !editing}
		<ButtonGroup class="ml-2 mb-1 h-[42px] self-center">
			<Button
				class="pr-3 pl-3 pt-1 pb-1"
				on:click={() => {
					dispatch('edit_device', { index: value });
				}}
				><svg
					xmlns="http://www.w3.org/2000/svg"
					viewBox="0 0 48 48"
					stroke="currentColor"
					fill="currentColor"
					class="w-5 h-5 text-purple-500 dark:text-green-500"
					><path
						d="m39.7 14.7-6.4-6.4 2.1-2.1q.85-.85 2.125-.825 1.275.025 2.125.875L41.8 8.4q.85.85.85 2.1t-.85 2.1Zm-2.1 2.1L12.4 42H6v-6.4l25.2-25.2Z"
					/></svg
				></Button
			>
			<Tooltip arrow={false}>{$t('devices.edit device')}</Tooltip>
			<Button
				class="pr-3 pl-3 pt-1 pb-1"
				on:click={() => {
					show_more = !show_more;
				}}
				><svg
					xmlns="http://www.w3.org/2000/svg"
					viewBox="0 96 960 960"
					stroke="currentColor"
					fill="currentColor"
					class="w-5 h-5 text-purple-500 dark:text-green-500"
				>
					{#if !show_more}
						<path d="M480 711 240 471l43-43 197 198 197-197 43 43-240 239Z" />
					{:else}
						<path d="m283 711-43-43 240-240 240 239-43 43-197-197-197 198Z" />
					{/if}
				</svg></Button
			>
			<Tooltip arrow={false}>{$t('devices.show user nodes')}</Tooltip>
		</ButtonGroup>
	{/if}
</div>
{#if !editing && show_more}
	{#each user_nodes as user_node, i}
		<div class="flex mt-2 ml-10">
			<Indicator
				size="xs"
				color="none"
				class="z-10 flex-none m-2 mr-4 bg-gray-800 dark:bg-gray-900 self-center"
			/>
			<Card
				padding="none"
				size="lg"
				class="p-2 mb-0.5 flex-1 w-full text-ellipsis overflow-hidden whitespace-nowrap"
			>
				{#if device_type === 'robot'}
					<p class="text-sm font-medium text-gray-900 truncate dark:text-white">
						{user_node['Name']} · {robot_datatype[user_node['Datatype']]}
						{#if 'Count' in user_node && user_node['Count'] > 1}
							· Count: {user_node['Count']}
						{/if}
					</p>
					<p class="text-sm text-gray-500 truncate dark:text-gray-400">
						{user_node['Parent']}<br />
						'{user_node['ReadCommand']['Command']}' · '{user_node['ReadCommand']['Match']}'
						{#if 'Writeable' in user_node && user_node['Writeable']}
							<br />{user_node['WriteCommand']['Command']}
						{/if}
					</p>
				{:else if device_type === 'plc'}
					<p class="text-sm font-medium text-gray-900 truncate dark:text-white">
						{user_node['Name']} · {user_node['Datatype']}
						{#if 'Count' in user_node && user_node['Count'] > 1}
							· Count: {user_node['Count']}
						{/if}
					</p>
					<p class="text-sm text-gray-500 truncate dark:text-gray-400">
						{user_node['Parent']}<br />
						{plc_variable_type[user_node['Type']]}:
						{#if user_node['Type'] === 'Device'}
							{plc_device_type[user_node['ReadCommand']['Device']]} · {user_node['ReadCommand'][
								'Head no'
							]}
						{:else if user_node['Type'] === 'GlobalLabel'}
							{user_node['ReadCommand']['Label']}
						{/if}
					</p>
				{/if}
			</Card>
			<ButtonGroup class="ml-2 mb-0.5 h-[42px] self-center">
				<Button
					class="pr-1 pl-3 pt-1 pb-1"
					on:click={async () => {
						await invoke('remove_user_node', {
							deviceName: device['Name'],
							userNodeName: user_node['Name'],
							userNodeParent: user_node['Parent']
						});
						dispatch('remove_user_node', { index: i });
					}}
					><svg
						xmlns="http://www.w3.org/2000/svg"
						viewBox="0 0 48 48"
						fill="currentColor"
						stroke="currentColor"
						class="w-5 h-5 mr-2 text-purple-500 dark:text-green-500"
						><path
							d="M13.05 42q-1.25 0-2.125-.875T10.05 39V10.5H8v-3h9.4V6h13.2v1.5H40v3h-2.05V39q0 1.2-.9 2.1-.9.9-2.1.9Zm21.9-31.5h-21.9V39h21.9Zm-16.6 24.2h3V14.75h-3Zm8.3 0h3V14.75h-3Zm-13.6-24.2V39Z"
						/></svg
					></Button
				>
				<Tooltip arrow={false}>{$t('devices.delete user node')}</Tooltip>
			</ButtonGroup>
		</div>
	{/each}
	<ButtonGroup divClass="self-end mt-2 inline-flex rounded-lg shadow-sm">
		{#if device_type === 'plc'}
			<Button
				on:click={async () => {
					const selected = await open({
						multiple: false,
						directory: false,
						filters: [
							{
								name: 'CSV file (*.csv)',
								extensions: ['csv']
							}
						]
					});
					if (selected) {
						info(`Parsing csv file ${selected}`);
						let user_nodes = [];
						const contents = new TextDecoder('utf-16').decode(await readBinaryFile(selected));
						const lines = contents.split('\n');

						const parse_line = (line) => line.slice(1, -1).split('"\t"');

						const headers = parse_line(lines[1]);

						for (const line of lines.slice(2)) {
							const elements = parse_line(line);
							const label = elements[headers.indexOf('Label Name')];
							const datatype = elements[headers.indexOf('Data Type')];
							const access_from_external_device =
								elements[headers.indexOf('Access from External Device')];
							if (access_from_external_device != '1') continue;

							if (!plc_datatype.hasOwnProperty(datatype)) {
								// not supported datatype
								warn(`Global label '${label}' has unsupported datatype '${datatype}'`);
								continue;
							}

							const user_node = {
								Datatype: plc_datatype[datatype],
								Name: label,
								Parent: 'Global Label',
								ReadCommand: {
									Label: label
								},
								Type: 'GlobalLabel',
								Writeable: true
							};
							let answer = await invoke('add_user_node', {
								deviceName: device['Name'],
								userNode: JSON.stringify(user_node)
							});
							if (answer === null) {
								info(`Adding Global label '${label}'`);
								user_nodes.push(user_node);
							}
						}
						dispatch('add_user_nodes', { user_nodes: user_nodes });
					}
				}}
			>
				<svg
					xmlns="http://www.w3.org/2000/svg"
					viewBox="0 96 960 960"
					fill="currentColor"
					stroke="currentColor"
					class="w-5 h-5 mr-2 text-purple-500 dark:text-green-500"
					><path
						d="M450 896V525L330 645l-43-43 193-193 193 193-43 43-120-120v371h-60ZM160 459V316q0-24 18-42t42-18h520q24 0 42 18t18 42v143h-60V316H220v143h-60Z"
					/></svg
				>
				{$t('devices.import global label file')}
			</Button>
		{/if}
		<Button
			on:click={() => {
				dispatch('show_user_node_form', device['Name']);
			}}
		>
			<svg
				xmlns="http://www.w3.org/2000/svg"
				viewBox="0 0 48 48"
				fill="currentColor"
				stroke="currentColor"
				class="w-5 h-5 mr-2 text-purple-500 dark:text-green-500"
				><path d="M22.5 38V25.5H10v-3h12.5V10h3v12.5H38v3H25.5V38Z" /></svg
			>
			{$t('devices.add user node')}
		</Button>
	</ButtonGroup>
{/if}
