<script>
	import ThirdPartyLicenses from '$lib/ThirdPartyLicenses.svelte';
	import { Tabs, TabItem } from 'flowbite-svelte';
	import { t } from '$lib/translations/translations';
</script>

<Tabs
	contentClass="mt-4 mb-4"
	activeClasses="p-4 text-blue-600 bg-gray-200 rounded-t-lg dark:bg-gray-800 dark:text-blue-500"
>
	<TabItem open contentClass="">
		<span slot="title">Aerion OPC-UA Server</span>
		<pre class="mb-2 text-gray-500 dark:text-gray-400">Aerion OPC-UA Server vversion-number</pre>
	</TabItem>
	<TabItem contentClass="">
		<span slot="title">{$t('about.third party licenses')}</span>
		<ThirdPartyLicenses />
	</TabItem>
</Tabs>
