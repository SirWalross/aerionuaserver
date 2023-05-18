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
		Helper
	} from 'flowbite-svelte';
	import { t } from '$lib/translations/translations';
	import { open } from '@tauri-apps/api/shell';
	import { invoke } from '@tauri-apps/api/tauri';
	import { createEventDispatcher } from 'svelte';

	export let device_name: string;

	const dispatch = createEventDispatcher();

	let name: string;
	let parent_name: string;
	let read_command_command: string;
	let read_command_match: string;
	let datatype: string;
	let writeable: string[] = [];
	let write_command_command: string = '';

	let submit_error_msg: string = '';
</script>

<Modal size="xs" autoclose={false} class="w-full" open={true} on:hide>
	<form
		class="flex flex-col space-y-6"
		action="#"
		on:submit={async (e) => {
			if (e?.submitter?.id == 'add user node') {
				let user_node = {
					Name: name,
					Parent: parent_name,
					Datatype: datatype,
					ReadCommand: {
						Command: read_command_command,
						Match: read_command_match
					},
					Writeable: writeable.length !== 0,
					WriteCommand: {
						Command: write_command_command
					}
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
				<Helper class="mt-1" color="red"
					><span class="font-medium">{$t('devices.form.error')}: {submit_error_msg}</span></Helper
				>
			{/if}
		</Label>
		<Label class="space-y-2">
			<span>{$t('devices.form.parent node name')}</span>
			<Input type="text" name="name" bind:value={parent_name} required />
		</Label>
		<Label class="space-y-2">
			<span>{$t('devices.form.read command command')}</span>
			<Input type="text" name="name" bind:value={read_command_command} required />
		</Label>
		<Label class="space-y-2">
			<span>{$t('devices.form.read command match')}</span>
			<Input type="text" name="name" bind:value={read_command_match} required />
		</Label>
		<Label class="space-y-2">
			<span>{$t('devices.form.datatype')}</span>
			<Select bind:value={datatype} placeholder="">
				<option selected value="Position">Position</option>
				<option value="Joint">Joint</option>
				<option value="Int32">Integer</option>
				<option value="Double">Double</option>
				<option value="HexInt32">Hexadecimal Integer</option>
				<option value="Bool">Boolean</option>
				<option value="String">String</option>
			</Select>
		</Label>
		<Checkbox class="mb-6 space-x-1" value={1} bind:group={writeable}
			>{$t('devices.form.writeable')}</Checkbox
		>
		{#if writeable.length}
			<Label class="space-y-2">
				<span>{$t('devices.form.write command command')}</span>
				<Input type="text" name="name" bind:value={write_command_command} required />
			</Label>
		{/if}
		<ButtonGroup class="space-x-px">
			<Button
				color="blue"
				class="!p-2"
				on:click={() => {
					open('https://github.com/SirWalross/aerionuaserver/blob/gui/RobotUserNodeFormat.md');
				}}
			>
				<svg
					aria-hidden="true"
					class="w-5 h-5"
					fill="currentColor"
					viewBox="0 96 960 960"
					xmlns="http://www.w3.org/2000/svg"
					><path
						d="M484 809q16 0 27-11t11-27q0-16-11-27t-27-11q-16 0-27 11t-11 27q0 16 11 27t27 11Zm-35-146h59q0-26 6.5-47.5T555 566q31-26 44-51t13-55q0-53-34.5-85T486 343q-49 0-86.5 24.5T345 435l53 20q11-28 33-43.5t52-15.5q34 0 55 18.5t21 47.5q0 22-13 41.5T508 544q-30 26-44.5 51.5T449 663Zm31 313q-82 0-155-31.5t-127.5-86Q143 804 111.5 731T80 576q0-83 31.5-156t86-127Q252 239 325 207.5T480 176q83 0 156 31.5T763 293q54 54 85.5 127T880 576q0 82-31.5 155T763 858.5q-54 54.5-127 86T480 976Zm0-60q142 0 241-99.5T820 576q0-142-99-241t-241-99q-141 0-240.5 99T140 576q0 141 99.5 240.5T480 916Zm0-340Z"
					/></svg
				>
			</Button>
			<Tooltip arrow={false}>{$t('devices.form.information about format')}</Tooltip>
			<Button color="blue" type="submit" class="w-full1 !p-2 w-full" id="add user node">
				{$t('devices.form.add user node')}
				{name}
			</Button>
		</ButtonGroup>
	</form>
</Modal>
