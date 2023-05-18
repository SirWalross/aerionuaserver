<script>
	import {
		Table,
		TableBody,
		TableBodyCell,
		TableBodyRow,
		TableHead,
		TableHeadCell
	} from 'flowbite-svelte';
	import { invoke } from '@tauri-apps/api/tauri';
	import { t } from '$lib/translations/translations';

	let interfaces_promise = invoke('get_network_interfaces').then((value) => JSON.parse(value));
</script>

<Table>
	<TableHead>
		<TableHeadCell>{$t('interfaces.name')}</TableHeadCell>
		<TableHeadCell>{$t('interfaces.ipv4')}</TableHeadCell>
		<TableHeadCell>{$t('interfaces.netmask')}</TableHeadCell>
		<TableHeadCell>{$t('interfaces.ipv6')}</TableHeadCell>
		<TableHeadCell>{$t('interfaces.netmask')}</TableHeadCell>
	</TableHead>
	{#await interfaces_promise then interfaces}
		{#each interfaces as interface_}
			<TableBodyRow>
				<TableBodyCell>{interface_['name']}</TableBodyCell>
				<TableBodyCell>{interface_['ipv4-addr']['address'].replace(':0', '')}</TableBodyCell>
				<TableBodyCell>{interface_['ipv4-addr']['netmask'].replace(':0', '')}</TableBodyCell>
				<TableBodyCell
					>{interface_['ipv6-addr']['address']
						.replace(']:0', '')
						.replace('[', '')
						.split('%')[0]}</TableBodyCell
				>
				<TableBodyCell>{interface_['ipv6-addr']['netmask'].replace(':0', '')}</TableBodyCell>
			</TableBodyRow>
		{/each}
	{/await}
</Table>
