<script type="ts">
	import { t } from '$lib/translations/translations';
	import { type } from '@tauri-apps/api/os';
	import { invoke } from '@tauri-apps/api/tauri';
	import { Button, Label, NumberInput, Toast, Checkbox, Helper } from 'flowbite-svelte';

	export let data;

	let port: number = data.port;
	let show_warning: boolean = false;

	let security_policies: string[] = data.security_policies;
	let security_modes: string[] = data.security_modes;
	let verify_client_certificiates: number[] = data.enforce_client_certificate_verification;
	$: selected_security_modes_invalid =
		security_modes.length !== 0 &&
		security_policies.length === 1 &&
		security_policies[0] === 'None';
	$: selected_verification_invalid =
		verify_client_certificiates.length !== 0 &&
		verify_client_certificiates[0] == 1 &&
		security_policies.length === 1 &&
		security_policies[0] === 'None';
</script>

<form
	class="flex flex-col space-y-6 w-[60rem]"
	action="#"
	on:submit={async (e) => {
		if (
			security_policies.length !== 0 &&
			!selected_security_modes_invalid &&
			!selected_verification_invalid
		) {
			await invoke('write_to_server_file', {
				key: 'Port',
				value: port.toString(),
				valueType: 'Number'
			});
			await invoke('write_to_server_file', {
				key: 'SecurityPolicies',
				value: JSON.stringify(security_policies),
				valueType: 'Array'
			});
			await invoke('write_to_server_file', {
				key: 'SecurityModes',
				value: JSON.stringify(security_modes),
				valueType: 'Array'
			});
			await invoke('write_to_server_file', {
				key: 'EnforceClientCertificateVerification',
				value: JSON.stringify(
					verify_client_certificiates.length !== 0 && verify_client_certificiates[0] === 1
				),
				valueType: 'Boolean'
			});
			show_warning = true;
		}
	}}
>
	<h3 class="text-xl font-medium text-gray-900 dark:text-white p-0">
		{$t('settings.settings')}
	</h3>
	<Label class="space-y-2">
		<span>{$t('settings.opcua port')}</span>
		<NumberInput bind:value={port} required />
	</Label>

	<Label class="space-y-2">
		<span>{$t('settings.security policies')}</span>
		<ul
			class="items-center w-full rounded-lg border {security_policies.length === 0
				? 'border-red-400'
				: 'border-gray-200'} sm:flex dark:bg-gray-800 {security_policies.length === 0
				? 'dark:border-red-600'
				: 'dark:border-gray-600'} divide-x divide-gray-200 dark:divide-gray-600"
		>
			<li class="w-full">
				<Checkbox class="p-3" bind:group={security_policies} value={'None'}>None</Checkbox>
			</li>
			<li class="w-full">
				<Checkbox class="p-3" bind:group={security_policies} value={'Basic256Sha256'}
					>Basic256Sha256</Checkbox
				>
			</li>
			<li class="w-full">
				<Checkbox class="p-3" bind:group={security_policies} value={'Aes128Sha256RsaOaep'}
					>Aes128Sha256RsaOaep</Checkbox
				>
			</li>
		</ul>
		{#if security_policies.length === 0}
			<Helper class="ml-4 mt-2" color="red"
				>{$t('settings.select at least one security policy')}</Helper
			>
		{/if}
	</Label>

	<Label class="space-y-2">
		<span>{$t('settings.security modes')}</span>
		<ul
			class="items-center w-full rounded-lg border {selected_security_modes_invalid
				? 'border-red-400'
				: 'border-gray-200'} sm:flex dark:bg-gray-800 {selected_security_modes_invalid
				? 'dark:border-red-600'
				: 'dark:border-gray-600'} divide-x divide-gray-200 dark:divide-gray-600"
		>
			<li class="w-full">
				<Checkbox class="p-3" bind:group={security_modes} value={'Sign'}>Sign</Checkbox>
			</li>
			<li class="w-full">
				<Checkbox class="p-3" bind:group={security_modes} value={'Sign & Encrypt'}
					>Sign & Encrypt</Checkbox
				>
			</li>
		</ul>
		{#if selected_security_modes_invalid}
			<Helper class="ml-4 mt-2" color="red"
				>{$t('settings.mode select at least one encrypted security policy')}</Helper
			>
		{/if}
	</Label>

	<Label class="space-y-2">
		<span>{$t('settings.client certificate verification')}</span>
		<Checkbox class="p-3" bind:group={verify_client_certificiates} value={1}
			>{$t('settings.enforce')}</Checkbox
		>
		{#if selected_verification_invalid}
			<Helper class="ml-4 mt-2" color="red"
				>{$t('settings.verification select at least one encrypted security policy')}</Helper
			>
		{:else if verify_client_certificiates.length !== 0 && verify_client_certificiates[0] === 1}
			{#await type() then _type}
				{#if _type === 'Linux'}
					<Helper class="ml-4 mt-1" color="disabled"
						>{$t('settings.place certificates in folder.prefix')}
						<div class="text-gray-900 dark:text-white inline-flex">
							~/.aerionuaserver/server/trusted
						</div>
						{$t('settings.place certificates in folder.suffix')}</Helper
					>
				{:else}
					<Helper class="ml-4 mt-1" color="disabled"
						>{$t('settings.place certificates in folder.prefix')}
						<div class="text-gray-900 dark:text-white inline-flex">
							%appdata%\Aerion OPC-UA Server\server\trusted
						</div>
						{$t('settings.place certificates in folder.suffix')}</Helper
					>
				{/if}
			{/await}
		{/if}
	</Label>

	<Button
		color="blue"
		type="submit"
		class="w-full1 !p-2 w-full text-ellipsis overflow-hidden whitespace-nowrap"
		id="apply"
	>
		{$t('settings.apply')}
	</Button>
</form>

<Toast class="absolute inset-x-0 bottom-20 self-center m-5" color="red" bind:open={show_warning}>
	<svelte:fragment slot="icon">
		<svg
			aria-hidden="true"
			class="w-5 h-5"
			fill="currentColor"
			viewBox="0 0 20 20"
			xmlns="http://www.w3.org/2000/svg"
			><path
				fill-rule="evenodd"
				d="M8.257 3.099c.765-1.36 2.722-1.36 3.486 0l5.58 9.92c.75 1.334-.213 2.98-1.742 2.98H4.42c-1.53 0-2.493-1.646-1.743-2.98l5.58-9.92zM11 13a1 1 0 11-2 0 1 1 0 012 0zm-1-8a1 1 0 00-1 1v3a1 1 0 002 0V6a1 1 0 00-1-1z"
				clip-rule="evenodd"
			/></svg
		>
		<span class="sr-only">Warning icon</span>
	</svelte:fragment>
	{$t('settings.restart server')}
</Toast>
