<script lang="ts">
	import { Card, ButtonGroup, Button } from 'flowbite-svelte';
	import { createEventDispatcher, tick } from 'svelte';
	import { t } from '$lib/translations/translations';

	const dispatch = createEventDispatcher();

	export let name: string;
	export let set_form: (event: any) => void;
	export let disabled_remove: boolean;

	let show_add_device: boolean = false;
	let _show_user_node_form: boolean = false;
	let editing: boolean = false;
	let group: number[] = [];
	let current_device_for_user_node: string;

	let hide_add_device = () => {
		show_add_device = false;
	};

	let hide_user_node_form = () => {
		_show_user_node_form = false;
	};

	let change_checkmark = (event) => {
		const index = group.indexOf(event.detail.index);
		if (index < 0) {
			group.push(event.detail.index);
		} else {
			group.splice(index, 1);
		}
		group = group;
	};

	let edit_device = async (event) => {
		show_add_device = true;
		await tick();
		set_form(event);
	};

	let show_user_node_form = async (event) => {
		current_device_for_user_node = event.detail;
		await tick();
		_show_user_node_form = true;
	};
</script>

<Card class="mb-2 mx-auto" size="lg">
	<h5 class="mb-2 text-2xl font-bold tracking-tight text-gray-900 dark:text-white">{name}</h5>
	<slot {editing} {change_checkmark} {edit_device} {show_user_node_form} />
	<ButtonGroup divClass="mt-3 inline-flex rounded-lg shadow-sm">
		{#if !editing}
			<Button on:click={() => (show_add_device = true)}>
				<svg
					xmlns="http://www.w3.org/2000/svg"
					viewBox="0 0 48 48"
					fill="currentColor"
					stroke="currentColor"
					class="w-5 h-5 mr-2 text-purple-500 dark:text-green-500"
					><path d="M22.5 38V25.5H10v-3h12.5V10h3v12.5H38v3H25.5V38Z" /></svg
				>
				{$t('devices.add')}
			</Button>
			<Button
				disabled={disabled_remove || null}
				on:click={() => {
					editing = true;
				}}
			>
				<svg
					xmlns="http://www.w3.org/2000/svg"
					viewBox="0 0 48 48"
					fill="currentColor"
					stroke="currentColor"
					class="w-5 h-5 mr-2 text-purple-500 dark:text-green-500"
					><path
						d="M13.05 42q-1.25 0-2.125-.875T10.05 39V10.5H8v-3h9.4V6h13.2v1.5H40v3h-2.05V39q0 1.2-.9 2.1-.9.9-2.1.9Zm21.9-31.5h-21.9V39h21.9Zm-16.6 24.2h3V14.75h-3Zm8.3 0h3V14.75h-3Zm-13.6-24.2V39Z"
					/></svg
				>
				{$t('devices.remove')}
			</Button>
		{:else}
			<Button
				on:click={() => {
					editing = false;
					group = [];
				}}
			>
				<svg
					xmlns="http://www.w3.org/2000/svg"
					viewBox="0 0 48 48"
					fill="currentColor"
					stroke="currentColor"
					class="w-5 h-5 mr-2 text-purple-500 dark:text-green-500"
					><path
						d="m12.45 37.65-2.1-2.1L21.9 24 10.35 12.45l2.1-2.1L24 21.9l11.55-11.55 2.1 2.1L26.1 24l11.55 11.55-2.1 2.1L24 26.1Z"
					/></svg
				>
				{$t('devices.cancel')}
			</Button>
			<Button
				on:click={async () => {
					editing = false;
					if (group.length > 0) {
						await dispatch('remove', group);
					}
					group = [];
				}}
			>
				<svg
					xmlns="http://www.w3.org/2000/svg"
					viewBox="0 0 48 48"
					fill="currentColor"
					stroke="currentColor"
					class="w-5 h-5 mr-2 text-red-500 dark:text-red-500"
					><path
						d="M18.05 33.05 24 27l6 6.05 2.35-2.4-5.95-6.05 5.95-6.05-2.35-2.4-6 6.05-5.95-6.05-2.4 2.4 6 6.05-6 6.05Zm-5 8.95q-1.2 0-2.1-.9-.9-.9-.9-2.1V10.5H8v-3h9.4V6h13.2v1.5H40v3h-2.05V39q0 1.2-.9 2.1-.9.9-2.1.9Zm21.9-31.5h-21.9V39h21.9Zm-21.9 0V39Z"
					/></svg
				>
				{$t('devices.remove')}
			</Button>
		{/if}
	</ButtonGroup>
</Card>
{#if show_add_device}
	<slot name="DeviceForm" {hide_add_device} />
{/if}
{#if _show_user_node_form}
	<slot name="UserNodeForm" {hide_user_node_form} {current_device_for_user_node} />
{/if}
