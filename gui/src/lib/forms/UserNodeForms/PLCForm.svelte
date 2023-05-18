<script lang="ts">
	import {
		Button,
		Modal,
		Label,
		Input,
		Select,
		Checkbox,
		ButtonGroup,
		Tooltip,
		Helper,
		NumberInput
	} from 'flowbite-svelte';
	import { t } from '$lib/translations/translations';
	import { open } from '@tauri-apps/api/shell';
	import { invoke } from '@tauri-apps/api/tauri';
	import { createEventDispatcher } from 'svelte';

	export let device_name: string;

	const dispatch = createEventDispatcher();

	let name: string;
	let parent_name: string;
	let variable_type: string;
	let datatype: string;
	let writeable: string[] = [];
	let count: number = 1;
	let device: string;
	let length: number = 1;
	let start_io_number: string; // in hex
	let head_no: number;
	let label_name: string;

	let submit_error_msg: string = '';
</script>

<Modal size="xs" autoclose={false} class="w-full" open={true} on:hide>
	<form
		class="flex flex-col space-y-6"
		action="#"
		on:submit={async (e) => {
			if (e?.submitter?.id == 'add user node') {
				let device_ = device === 'G' ? `U${start_io_number.padStart(3, '0')}` : device;
				let user_node = {
					Name: name,
					Parent: parent_name,
					Type: variable_type,
					Datatype: datatype,
					ReadCommand: {
						...(variable_type === 'GlobalLabel' && { Label: label_name }),
						...(variable_type === 'Device' && { Device: device_, 'Head no': head_no }),
						...(variable_type === 'Device' && datatype === 'String' && { Length: length })
					},
					Writeable: writeable.length !== 0,
					Count: count
				};
				let answer = await invoke('add_user_node', {
					deviceName: device_name,
					userNode: JSON.stringify(user_node)
				});
				if (answer === null) {
					dispatch('hide');
					dispatch('add', { device_name: device_name, user_node: user_node });
				} else {
					submit_error_msg = answer;
				}
			}
		}}
		on:change={() => {
			submit_error_msg = '';
		}}
	>
		<h3 class="text-xl font-medium text-gray-900 dark:text-white p-0">
			{$t('devices.form.add a user node')}
		</h3>
		<Label class="space-y-2">
			<span>{$t('devices.form.user node name')}</span>
			<Input type="text" name="name" bind:value={name} required />
			{#if submit_error_msg !== ''}
				<Helper class="mt-1" color="red">
					<span class="font-medium">{$t('devices.form.error')}: {submit_error_msg}</span>
				</Helper>
			{/if}
		</Label>
		<Label class="space-y-2">
			<span>{$t('devices.form.parent node name')}</span>
			<Input type="text" name="name" bind:value={parent_name} required />
		</Label>
		<Label class="space-y-2">
			<span>{$t('devices.form.variable type')}</span>
			<Select bind:value={variable_type} placeholder="">
				<option selected value="GlobalLabel">Global Label</option>
				<option value="Device">Device</option>
			</Select>
		</Label>
		{#if variable_type === 'GlobalLabel'}
			<Label class="space-y-2">
				<span>{$t('devices.form.label name')}</span>
				<Input type="text" name="name" bind:value={label_name} required />
			</Label>
		{:else if variable_type === 'Device'}
			<Label class="space-y-2">
				<span>{$t('devices.form.device')}</span>
				<Select bind:value={device} placeholder="">
					<option value="X">X</option>
					<option value="Y">Y</option>
					<option value="M">M</option>
					<option selected value="D">D</option>
					<option value="W">W</option>
					<option value="SM">SM</option>
					<option value="SD">SD</option>
					<option value="L">L</option>
					<option value="U3E0">U3E0</option>
					<option value="U3E1">U3E1</option>
					<option value="U3E2">U3E2</option>
					<option value="U3E3">U3E3</option>
					<option value="G">G</option>
				</Select>
			</Label>
			{#if device == 'G'}
				<Label class="space-y-2">
					<span>{$t('devices.form.start i/o number.main')}</span>
					<br />
					<span class="text-ls text-gray-500 dark:text-gray-400"
						>{$t('devices.form.start i/o number.info')}</span
					>
					<Input bind:value={start_io_number} required />
				</Label>
			{/if}
			<Label class="space-y-2">
				<span>{$t('devices.form.head no')}</span>
				<NumberInput bind:value={head_no} required />
			</Label>
		{/if}
		<Label class="space-y-2">
			<span>{$t('devices.form.datatype')}</span>
			<Select bind:value={datatype} placeholder="">
				<option selected value="Word">Word</option>
				<option value="DWord">Double Word</option>
				<option value="Int">Signed Word</option>
				<option value="DInt">Signed Double Word</option>
				<option value="Float">Float</option>
				<option value="Double">Double</option>
				<option value="Bool">Bool</option>
				<option value="String">String</option>
			</Select>
		</Label>
		{#if datatype === 'String' && variable_type === 'Device'}
			<Label class="space-y-2">
				<span>{$t('devices.form.length')}</span>
				<NumberInput bind:value={length} required />
			</Label>
		{/if}
		<Label class="space-y-2">
			<span>{$t('devices.form.count')}</span>
			<NumberInput bind:value={count} required />
		</Label>
		<Checkbox class="mb-6 space-x-1" value={1} bind:group={writeable}
			>{$t('devices.form.writeable')}</Checkbox
		>
		<ButtonGroup class="space-x-px">
			<Button
				color="blue"
				class="!p-2"
				on:click={() => {
					open('https://github.com/SirWalross/aerionuaserver/blob/gui/PLCUserNodeFormat.md');
				}}
			>
				<svg
					aria-hidden="true"
					class="w-5 h-5"
					fill="currentColor"
					viewBox="0 96 960 960"
					xmlns="http://www.w3.org/2000/svg"
				>
					<path
						d="M484 809q16 0 27-11t11-27q0-16-11-27t-27-11q-16 0-27 11t-11 27q0 16 11 27t27 11Zm-35-146h59q0-26 6.5-47.5T555 566q31-26 44-51t13-55q0-53-34.5-85T486 343q-49 0-86.5 24.5T345 435l53 20q11-28 33-43.5t52-15.5q34 0 55 18.5t21 47.5q0 22-13 41.5T508 544q-30 26-44.5 51.5T449 663Zm31 313q-82 0-155-31.5t-127.5-86Q143 804 111.5 731T80 576q0-83 31.5-156t86-127Q252 239 325 207.5T480 176q83 0 156 31.5T763 293q54 54 85.5 127T880 576q0 82-31.5 155T763 858.5q-54 54.5-127 86T480 976Zm0-60q142 0 241-99.5T820 576q0-142-99-241t-241-99q-141 0-240.5 99T140 576q0 141 99.5 240.5T480 916Zm0-340Z"
					/>
				</svg>
			</Button>
			<Tooltip arrow={false}>{$t('devices.form.information about format')}</Tooltip>
			<Button color="blue" type="submit" class="w-full1 !p-2 w-full" id="add user node">
				{$t('devices.form.add user node')}
				{name}
			</Button>
		</ButtonGroup>
	</form>
</Modal>
