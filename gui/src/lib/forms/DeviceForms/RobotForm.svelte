<script lang="ts">
	import {
		ButtonGroup,
		Button,
		Modal,
		Label,
		Input,
		Helper,
		Tooltip,
		Spinner,
		NumberInput
	} from 'flowbite-svelte';
	import { invoke } from '@tauri-apps/api/tauri';
	import { createEventDispatcher } from 'svelte';
	import { isIPv4 } from 'is-ip';
	import { t } from '$lib/translations/translations';

	const dispatch = createEventDispatcher();

	let editing = null;
	let robname: string;
	let ip_addr: string;
	let port: number = 10001;
	let valid_ip: boolean = true;
	let checking_connection_promise = new Promise((_, reject) => {
		reject();
	});
	let submit_error_msg: string = '';
	export const set_form = (robot) => {
		editing = { original_name: robot['Name'], user_nodes: robot['UserNodes'] };
		robname = robot['Name'];
		ip_addr = robot['Ip'];
		port = robot['Port'];
	};
</script>

<Modal size="xs" autoclose={false} class="w-full" open={true} on:hide>
	<form
		class="flex flex-col space-y-6"
		action="#"
		on:submit={async (e) => {
			if (e?.submitter?.id == 'add robot button') {
				// so that pressing check connection doesnt invoke add_robot
				let robot = {
					Name: robname,
					Type: 'Robot',
					Ip: ip_addr,
					Port: port,
					UserNodes: []
				};
				if (editing !== null) {
					await invoke('remove_device', { device: editing['original_name'] });
					robot['UserNodes'] = editing['user_nodes'];
				}
				let answer = await invoke('add_device', { device: JSON.stringify(robot) });
				if (valid_ip && answer === null) {
					dispatch('hide');
					if (editing !== null) {
						robot['original_name'] = editing['original_name'];
						dispatch('edit', robot);
					} else {
						dispatch('add', robot);
					}
				} else {
					submit_error_msg = answer;
				}
			}
		}}
		on:input={() => {
			checking_connection_promise = new Promise((_, reject) => {
				reject();
			});
			submit_error_msg = '';
		}}
	>
		{#if editing !== null}
			<h3
				class="text-xl font-medium text-gray-900 dark:text-white p-0 text-ellipsis overflow-hidden whitespace-nowrap"
			>
				{$t('devices.form.edit')}
				{editing['original_name']}
			</h3>
		{:else}
			<h3 class="text-xl font-medium text-gray-900 dark:text-white p-0">
				{$t('devices.form.add a robot')}
			</h3>
		{/if}
		<Label class="space-y-2">
			<span>{$t('devices.form.name')}</span>
			<Input type="text" name="name" placeholder="Robot" bind:value={robname} required />
			{#if submit_error_msg !== ''}
				<Helper class="mt-1" color="red"
					><span class="font-medium">{$t('devices.form.error')}: {submit_error_msg}</span></Helper
				>
			{/if}
		</Label>
		<Label class="space-y-2">
			<span>{$t('devices.form.ip address')}</span>
			<Input
				type="text"
				name="ip"
				placeholder="192.168.0.20"
				required
				color="base"
				bind:value={ip_addr}
				on:change={(event) => {
					if (isIPv4(ip_addr)) {
						event.target.classList.remove(
							'focus:ring-red-500',
							'focus:border-red-500',
							'dark:focus:ring-red-500',
							'dark:focus:border-red-500',
							'bg-red-50',
							'text-red-900',
							'placeholder-red-700',
							'dark:text-red-500',
							'dark:placeholder-red-500',
							'dark:bg-gray-700',
							'border-red-500',
							'dark:border-red-400'
						);
						event.target.classList.add(
							'focus:ring-green-500',
							'focus:border-green-500',
							'dark:focus:border-green-500',
							'dark:focus:ring-green-500',
							'bg-green-50',
							'text-green-900',
							'placeholder-green-700',
							'dark:text-green-400',
							'dark:placeholder-green-500',
							'dark:bg-gray-700',
							'border-green-500',
							'dark:border-green-400'
						);
						valid_ip = true;
					} else {
						event.target.classList.remove(
							'focus:ring-green-500',
							'focus:border-green-500',
							'dark:focus:border-green-500',
							'dark:focus:ring-green-500',
							'bg-green-50',
							'text-green-900',
							'placeholder-green-700',
							'dark:text-green-400',
							'dark:placeholder-green-500',
							'dark:bg-gray-700',
							'border-green-500',
							'dark:border-green-400'
						);
						event.target.classList.add(
							'focus:ring-red-500',
							'focus:border-red-500',
							'dark:focus:ring-red-500',
							'dark:focus:border-red-500',
							'bg-red-50',
							'text-red-900',
							'placeholder-red-700',
							'dark:text-red-500',
							'dark:placeholder-red-500',
							'dark:bg-gray-700',
							'border-red-500',
							'dark:border-red-400'
						);
						valid_ip = false;
					}
				}}
			/>
			{#if !valid_ip}
				<Helper class="mt-1" color="red"
					><span class="font-medium">{$t('devices.form.invalid ip address')}</span></Helper
				>
			{/if}
		</Label>
		<Label class="space-y-2">
			<span>{$t('devices.form.port')}</span>
			<NumberInput bind:value={port} required />
		</Label>
		<ButtonGroup class="space-x-px">
			{#await checking_connection_promise}
				<Button color="blue" class="!p-2">
					<Spinner size="4" color="white" class="w-5 h-5" />
				</Button>
			{:then checking_connection}
				{#if checking_connection === null}
					<Button color="blue" class="!p-2 bg-green-600 dark:bg-green-600" id="connection-test">
						<svg
							aria-hidden="true"
							class="w-5 h-5 text-gray-100 dark:text-gray-800"
							fill="currentColor"
							viewBox="0 0 20 20"
							xmlns="http://www.w3.org/2000/svg"
							><path
								fill-rule="evenodd"
								d="M16.707 5.293a1 1 0 010 1.414l-8 8a1 1 0 01-1.414 0l-4-4a1 1 0 011.414-1.414L8 12.586l7.293-7.293a1 1 0 011.414 0z"
								clip-rule="evenodd"
							/></svg
						>
					</Button>
					<Tooltip triggeredBy="#connection-test" arrow={false}
						>{$t('devices.form.connection.successfull prefix')}{robname}{$t(
							'devices.form.connection.successfull suffix'
						)}</Tooltip
					>
				{:else}
					<Button color="blue" class="!p-2 bg-red-600 dark:bg-red-600" id="connection-test">
						<svg
							class="w-5 h-5 text-gray-100 dark:text-gray-800"
							fill="currentColor"
							viewBox="0 0 20 20"
							xmlns="http://www.w3.org/2000/svg"
							><path
								fill-rule="evenodd"
								d="M4.293 4.293a1 1 0 011.414 0L10 8.586l4.293-4.293a1 1 0 111.414 1.414L11.414 10l4.293 4.293a1 1 0 01-1.414 1.414L10 11.414l-4.293 4.293a1 1 0 01-1.414-1.414L8.586 10 4.293 5.707a1 1 0 010-1.414z"
								clip-rule="evenodd"
							/></svg
						>
					</Button>
					<Tooltip triggeredBy="#connection-test" arrow={false}>{checking_connection}</Tooltip>
				{/if}
			{:catch}
				<Button
					color="blue"
					class="!p-2"
					id="connection-test"
					type="submit"
					on:click={() => {
						let robot = { Name: robname, Type: 'Robot', Ip: ip_addr, Port: port };
						if (isIPv4(ip_addr)) {
							checking_connection_promise = invoke('check_connection', {
								device: JSON.stringify(robot)
							});
						}
					}}
				>
					<svg
						xmlns="http://www.w3.org/2000/svg"
						viewBox="0 0 48 48"
						fill="currentColor"
						stroke="currentColor"
						class="w-5 h-5"
						><path
							d="M9.85 38.15q-2.7-2.7-4.275-6.325Q4 28.2 4 24t1.575-7.825Q7.15 12.55 9.85 9.85L12 12q-2.3 2.3-3.65 5.375T7 24q0 3.55 1.325 6.6T12 36Zm5.65-5.65q-1.6-1.6-2.55-3.775Q12 26.55 12 24t.95-4.725Q13.9 17.1 15.5 15.5l2.15 2.15q-1.2 1.2-1.925 2.825Q15 22.1 15 24t.7 3.5q.7 1.6 1.95 2.85ZM24 28q-1.65 0-2.825-1.175Q20 25.65 20 24q0-1.65 1.175-2.825Q22.35 20 24 20q1.65 0 2.825 1.175Q28 22.35 28 24q0 1.65-1.175 2.825Q25.65 28 24 28Zm8.5 4.5-2.15-2.15q1.2-1.2 1.925-2.825Q33 25.9 33 24t-.7-3.5q-.7-1.6-1.95-2.85l2.15-2.15q1.6 1.6 2.55 3.775Q36 21.45 36 24q0 2.5-.95 4.675T32.5 32.5Zm5.65 5.65L36 36q2.3-2.3 3.65-5.375T41 24q0-3.55-1.325-6.6T36 12l2.15-2.15q2.7 2.75 4.275 6.375Q44 19.85 44 24t-1.575 7.775Q40.85 35.4 38.15 38.15Z"
						/></svg
					>
				</Button>
				<Tooltip triggeredBy="#connection-test" arrow={false}
					>{$t('devices.form.check connection')}</Tooltip
				>
			{/await}
			<Button
				color="blue"
				type="submit"
				class="w-full1 !p-2 w-full text-ellipsis overflow-hidden whitespace-nowrap"
				id="add robot button"
			>
				{#if robname !== ''}
					{$t('devices.form.add')} {robname}
				{:else}
					{$t('devices.form.add robot')}
				{/if}
			</Button>
		</ButtonGroup>
	</form>
</Modal>
