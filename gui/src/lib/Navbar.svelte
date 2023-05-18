<script lang="ts">
	import {
		Sidebar,
		SidebarGroup,
		SidebarItem,
		SidebarWrapper,
		Tooltip,
		DarkMode,
		Button,
		Dropdown,
		DropdownItem,
		Label,
		Input
	} from 'flowbite-svelte';
	import { page } from '$app/stores';
	import IconButtonDisabled from './IconButtonDisabled.svelte';
	import IconButton from './IconButton.svelte';
	import { server } from '$lib/stores/server';
	import { t, locale, locales } from '$lib/translations/translations';
	import { invoke } from '@tauri-apps/api/tauri';
	import { debug } from 'tauri-plugin-log-api';

	$: activeUrl = $page.url.pathname;
	let spanClass = 'flex-1 ml-3 whitespace-nowrap';
	let server_running: boolean;

	server.subscribe((value) => {
		server_running = value?.['running'] ?? false;
	});

	let languageDropdownOpen = false;
</script>

<Sidebar asideClass="w-[13rem] fixed top-0 overflow-x-hidden h-full shadow-2xl h-full">
	<SidebarWrapper class="h-full rounded-none w-[13rem] flex justify-between items-center flex-col">
		<SidebarGroup class="flex flex-wrap items-center">
			<SidebarItem
				class="w-[11rem]"
				label={$t('common.home')}
				href="/"
				active={activeUrl === '/'}
				{spanClass}
			>
				<svelte:fragment slot="icon">
					{#if activeUrl === '/'}
						<svg
							xmlns="http://www.w3.org/2000/svg"
							class="h-6 w-6"
							fill="currentColor"
							viewBox="0 0 48 48"><path d="M8 42V18L24 6l16 12v24H28V28h-8v14Z" /></svg
						>
					{:else}
						<svg
							xmlns="http://www.w3.org/2000/svg"
							class="h-6 w-6"
							fill="currentColor"
							viewBox="0 0 48 48"
							><path
								d="M11 39h7.5V26.5h11V39H37V19.5L24 9.75 11 19.5Zm-3 3V18L24 6l16 12v24H26.5V29.5h-5V42Zm16-17.65Z"
							/></svg
						>
					{/if}
				</svelte:fragment>
			</SidebarItem>
			<SidebarItem
				class="w-[11rem]"
				label={$t('common.devices')}
				href="/devices"
				active={activeUrl === '/devices'}
				{spanClass}
			>
				<svelte:fragment slot="icon">
					<svg
						xmlns="http://www.w3.org/2000/svg"
						class="h-6 w-6"
						fill="currentColor"
						viewBox="0 96 960 960"
						><path
							d="M80 896v-90h86V316q0-24.75 17.625-42.375T226 256h620v60H226v490h234v90H80Zm479 0q-16.575 0-27.787-13.5Q520 869 520 852V415q0-16.575 11.213-27.787Q542.425 376 559 376h271.751Q850 376 865 386.5q15 10.5 15 28.5v437.273q0 18.584-14.5 31.155Q851 896 831 896H559Zm21-90h240V436H580v370Z"
						/></svg
					>
				</svelte:fragment>
			</SidebarItem>
			<SidebarItem
				class="w-[11rem]"
				label={$t('common.logs')}
				href="/logs"
				active={activeUrl === '/logs'}
				{spanClass}
			>
				<svelte:fragment slot="icon">
					{#if activeUrl === '/logs'}
						<svg
							xmlns="http://www.w3.org/2000/svg"
							class="h-6 w-6"
							fill="currentColor"
							viewBox="0 0 48 48"
							><path
								d="M15.95 35.5h16.1v-3h-16.1Zm0-8.5h16.1v-3h-16.1ZM11 44q-1.2 0-2.1-.9Q8 42.2 8 41V7q0-1.2.9-2.1Q9.8 4 11 4h18.05L40 14.95V41q0 1.2-.9 2.1-.9.9-2.1.9Zm16.55-27.7H37L27.55 7Z"
							/></svg
						>
					{:else}
						<svg
							xmlns="http://www.w3.org/2000/svg"
							class="h-6 w-6"
							fill="currentColor"
							viewBox="0 0 48 48"
							><path
								d="M15.95 35.5h16.1v-3h-16.1Zm0-8.5h16.1v-3h-16.1ZM11 44q-1.2 0-2.1-.9Q8 42.2 8 41V7q0-1.2.9-2.1Q9.8 4 11 4h18.05L40 14.95V41q0 1.2-.9 2.1-.9.9-2.1.9Zm16.55-27.7V7H11v34h26V16.3ZM11 7v9.3V7v34V7Z"
							/></svg
						>
					{/if}
				</svelte:fragment>
			</SidebarItem>
			<SidebarItem
				class="w-[11rem]"
				label={$t('common.interfaces')}
				href="/interfaces"
				active={activeUrl === '/interfaces'}
				{spanClass}
			>
				<svelte:fragment slot="icon">
					<svg
						xmlns="http://www.w3.org/2000/svg"
						class="h-6 w-6"
						fill="currentColor"
						viewBox="0 0 48 48"
						><path
							d="m34 36-2.1-2.1 9.8-9.9-9.8-9.9L34 12l12 12Zm-20 0L2 24l12-12 2.1 2.1L6.3 24l9.8 9.9Zm2-10.25q-.7 0-1.225-.525Q14.25 24.7 14.25 24q0-.7.525-1.225.525-.525 1.225-.525.7 0 1.225.525.525.525.525 1.225 0 .7-.525 1.225-.525.525-1.225.525Zm8 0q-.7 0-1.225-.525Q22.25 24.7 22.25 24q0-.7.525-1.225.525-.525 1.225-.525.7 0 1.225.525.525.525.525 1.225 0 .7-.525 1.225-.525.525-1.225.525Zm8 0q-.7 0-1.225-.525Q30.25 24.7 30.25 24q0-.7.525-1.225.525-.525 1.225-.525.7 0 1.225.525.525.525.525 1.225 0 .7-.525 1.225-.525.525-1.225.525Z"
						/></svg
					>
				</svelte:fragment>
			</SidebarItem>
			<SidebarItem
				class="w-[11rem]"
				label={$t('common.settings')}
				href="/settings"
				active={activeUrl === '/settings'}
				{spanClass}
			>
				<svelte:fragment slot="icon">
					{#if activeUrl === '/settings'}
						<svg
							xmlns="http://www.w3.org/2000/svg"
							class="h-6 w-6"
							fill="currentColor"
							viewBox="0 0 48 48"
							><path
								d="m19.4 44-1-6.3q-.95-.35-2-.95t-1.85-1.25l-5.9 2.7L4 30l5.4-3.95q-.1-.45-.125-1.025Q9.25 24.45 9.25 24q0-.45.025-1.025T9.4 21.95L4 18l4.65-8.2 5.9 2.7q.8-.65 1.85-1.25t2-.9l1-6.35h9.2l1 6.3q.95.35 2.025.925Q32.7 11.8 33.45 12.5l5.9-2.7L44 18l-5.4 3.85q.1.5.125 1.075.025.575.025 1.075t-.025 1.05q-.025.55-.125 1.05L44 30l-4.65 8.2-5.9-2.7q-.8.65-1.825 1.275-1.025.625-2.025.925l-1 6.3ZM24 30.5q2.7 0 4.6-1.9 1.9-1.9 1.9-4.6 0-2.7-1.9-4.6-1.9-1.9-4.6-1.9-2.7 0-4.6 1.9-1.9 1.9-1.9 4.6 0 2.7 1.9 4.6 1.9 1.9 4.6 1.9Z"
							/></svg
						>
					{:else}
						<svg
							xmlns="http://www.w3.org/2000/svg"
							class="h-6 w-6"
							fill="currentColor"
							viewBox="0 0 48 48"
							><path
								d="m19.4 44-1-6.3q-.95-.35-2-.95t-1.85-1.25l-5.9 2.7L4 30l5.4-3.95q-.1-.45-.125-1.025Q9.25 24.45 9.25 24q0-.45.025-1.025T9.4 21.95L4 18l4.65-8.2 5.9 2.7q.8-.65 1.85-1.25t2-.9l1-6.35h9.2l1 6.3q.95.35 2.025.925Q32.7 11.8 33.45 12.5l5.9-2.7L44 18l-5.4 3.85q.1.5.125 1.075.025.575.025 1.075t-.025 1.05q-.025.55-.125 1.05L44 30l-4.65 8.2-5.9-2.7q-.8.65-1.825 1.275-1.025.625-2.025.925l-1 6.3ZM24 30.5q2.7 0 4.6-1.9 1.9-1.9 1.9-4.6 0-2.7-1.9-4.6-1.9-1.9-4.6-1.9-2.7 0-4.6 1.9-1.9 1.9-1.9 4.6 0 2.7 1.9 4.6 1.9 1.9 4.6 1.9Zm0-3q-1.45 0-2.475-1.025Q20.5 25.45 20.5 24q0-1.45 1.025-2.475Q22.55 20.5 24 20.5q1.45 0 2.475 1.025Q27.5 22.55 27.5 24q0 1.45-1.025 2.475Q25.45 27.5 24 27.5Zm0-3.5Zm-2.2 17h4.4l.7-5.6q1.65-.4 3.125-1.25T32.7 32.1l5.3 2.3 2-3.6-4.7-3.45q.2-.85.325-1.675.125-.825.125-1.675 0-.85-.1-1.675-.1-.825-.35-1.675L40 17.2l-2-3.6-5.3 2.3q-1.15-1.3-2.6-2.175-1.45-.875-3.2-1.125L26.2 7h-4.4l-.7 5.6q-1.7.35-3.175 1.2-1.475.85-2.625 2.1L10 13.6l-2 3.6 4.7 3.45q-.2.85-.325 1.675-.125.825-.125 1.675 0 .85.125 1.675.125.825.325 1.675L8 30.8l2 3.6 5.3-2.3q1.2 1.2 2.675 2.05Q19.45 35 21.1 35.4Z"
							/></svg
						>
					{/if}
				</svelte:fragment>
			</SidebarItem>
			<SidebarItem
				class="w-[11rem]"
				label={$t('common.about')}
				href="/about"
				active={activeUrl === '/about'}
				{spanClass}
			>
				<svelte:fragment slot="icon">
					{#if activeUrl === '/about'}
						<svg
							xmlns="http://www.w3.org/2000/svg"
							class="h-6 w-6"
							fill="currentColor"
							viewBox="0 0 48 48"
							><path
								d="M22.65 34h3V22h-3ZM24 18.3q.7 0 1.175-.45.475-.45.475-1.15t-.475-1.2Q24.7 15 24 15q-.7 0-1.175.5-.475.5-.475 1.2t.475 1.15q.475.45 1.175.45ZM24 44q-4.1 0-7.75-1.575-3.65-1.575-6.375-4.3-2.725-2.725-4.3-6.375Q4 28.1 4 23.95q0-4.1 1.575-7.75 1.575-3.65 4.3-6.35 2.725-2.7 6.375-4.275Q19.9 4 24.05 4q4.1 0 7.75 1.575 3.65 1.575 6.35 4.275 2.7 2.7 4.275 6.35Q44 19.85 44 24q0 4.1-1.575 7.75-1.575 3.65-4.275 6.375t-6.35 4.3Q28.15 44 24 44Z"
							/></svg
						>
					{:else}
						<svg
							xmlns="http://www.w3.org/2000/svg"
							class="h-6 w-6"
							fill="currentColor"
							viewBox="0 0 48 48"
							><path
								d="M22.65 34h3V22h-3ZM24 18.3q.7 0 1.175-.45.475-.45.475-1.15t-.475-1.2Q24.7 15 24 15q-.7 0-1.175.5-.475.5-.475 1.2t.475 1.15q.475.45 1.175.45ZM24 44q-4.1 0-7.75-1.575-3.65-1.575-6.375-4.3-2.725-2.725-4.3-6.375Q4 28.1 4 23.95q0-4.1 1.575-7.75 1.575-3.65 4.3-6.35 2.725-2.7 6.375-4.275Q19.9 4 24.05 4q4.1 0 7.75 1.575 3.65 1.575 6.35 4.275 2.7 2.7 4.275 6.35Q44 19.85 44 24q0 4.1-1.575 7.75-1.575 3.65-4.275 6.375t-6.35 4.3Q28.15 44 24 44Zm.05-3q7.05 0 12-4.975T41 23.95q0-7.05-4.95-12T24 7q-7.05 0-12.025 4.95Q7 16.9 7 24q0 7.05 4.975 12.025Q16.95 41 24.05 41ZM24 24Z"
							/></svg
						>
					{/if}
				</svelte:fragment>
			</SidebarItem>
		</SidebarGroup>
		<SidebarGroup border ulClass="bottom-5 w-[11rem]">
			<DarkMode class="text-lg" id="dark-mode-toggle">
				<svelte:fragment slot="lightIcon">
					<svg
						xmlns="http://www.w3.org/2000/svg"
						class="h-5 w-5"
						fill="currentColor"
						viewBox="0 0 48 48"
						><path
							d="M24 42q-7.5 0-12.75-5.25T6 24q0-7.5 5.25-12.75T24 6q.4 0 .85.025.45.025 1.15.075-1.8 1.6-2.8 3.95-1 2.35-1 4.95 0 4.5 3.15 7.65Q28.5 25.8 33 25.8q2.6 0 4.95-.925T41.9 22.3q.05.6.075.975Q42 23.65 42 24q0 7.5-5.25 12.75T24 42Zm0-3q5.45 0 9.5-3.375t5.05-7.925q-1.25.55-2.675.825Q34.45 28.8 33 28.8q-5.75 0-9.775-4.025T19.2 15q0-1.2.25-2.575.25-1.375.9-3.125-4.9 1.35-8.125 5.475Q9 18.9 9 24q0 6.25 4.375 10.625T24 39Zm-.2-14.85Z"
						/></svg
					>
				</svelte:fragment>
				<svelte:fragment slot="darkIcon">
					<svg
						xmlns="http://www.w3.org/2000/svg"
						class="h-5 w-5"
						fill="currentColor"
						viewBox="0 0 48 48"
						><path
							d="M24 31q2.9 0 4.95-2.05Q31 26.9 31 24q0-2.9-2.05-4.95Q26.9 17 24 17q-2.9 0-4.95 2.05Q17 21.1 17 24q0 2.9 2.05 4.95Q21.1 31 24 31Zm0 3q-4.15 0-7.075-2.925T14 24q0-4.15 2.925-7.075T24 14q4.15 0 7.075 2.925T34 24q0 4.15-2.925 7.075T24 34ZM3.5 25.5q-.65 0-1.075-.425Q2 24.65 2 24q0-.65.425-1.075Q2.85 22.5 3.5 22.5h5q.65 0 1.075.425Q10 23.35 10 24q0 .65-.425 1.075-.425.425-1.075.425Zm36 0q-.65 0-1.075-.425Q38 24.65 38 24q0-.65.425-1.075.425-.425 1.075-.425h5q.65 0 1.075.425Q46 23.35 46 24q0 .65-.425 1.075-.425.425-1.075.425ZM24 10q-.65 0-1.075-.425Q22.5 9.15 22.5 8.5v-5q0-.65.425-1.075Q23.35 2 24 2q.65 0 1.075.425.425.425.425 1.075v5q0 .65-.425 1.075Q24.65 10 24 10Zm0 36q-.65 0-1.075-.425-.425-.425-.425-1.075v-5q0-.65.425-1.075Q23.35 38 24 38q.65 0 1.075.425.425.425.425 1.075v5q0 .65-.425 1.075Q24.65 46 24 46ZM12 14.1l-2.85-2.8q-.45-.45-.425-1.075.025-.625.425-1.075.45-.45 1.075-.45t1.075.45L14.1 12q.4.45.4 1.05 0 .6-.4 1-.4.45-1.025.45-.625 0-1.075-.4Zm24.7 24.75L33.9 36q-.4-.45-.4-1.075t.45-1.025q.4-.45 1-.45t1.05.45l2.85 2.8q.45.45.425 1.075-.025.625-.425 1.075-.45.45-1.075.45t-1.075-.45ZM33.9 14.1q-.45-.45-.45-1.05 0-.6.45-1.05l2.8-2.85q.45-.45 1.075-.425.625.025 1.075.425.45.45.45 1.075t-.45 1.075L36 14.1q-.4.4-1.025.4-.625 0-1.075-.4ZM9.15 38.85q-.45-.45-.45-1.075t.45-1.075L12 33.9q.45-.45 1.05-.45.6 0 1.05.45.45.45.45 1.05 0 .6-.45 1.05l-2.8 2.85q-.45.45-1.075.425-.625-.025-1.075-.425ZM24 24Z"
						/></svg
					>
				</svelte:fragment>
			</DarkMode>
			<Tooltip triggeredBy="#dark-mode-toggle" arrow={false}>
				{#if window.document.documentElement.classList.contains('dark')}
					{$t('common.light mode')}
				{:else}
					{$t('common.dark mode')}
				{/if}
			</Tooltip>
			<IconButtonDisabled class="text-lg" id="start-stop-server">
				{#if server_running}
					<svg
						xmlns="http://www.w3.org/2000/svg"
						class="h-5 w-5 text-green-400 dark:text-green-600"
						fill="currentColor"
						viewBox="0 96 960 960"
						><path
							d="M286.882 339Q266 339 251.5 353.618q-14.5 14.617-14.5 35.5Q237 410 251.618 424.5q14.617 14.5 35.5 14.5Q308 439 322.5 424.382q14.5-14.617 14.5-35.5Q337 368 322.382 353.5q-14.617-14.5-35.5-14.5Zm0 414Q266 753 251.5 767.618q-14.5 14.617-14.5 35.5Q237 824 251.618 838.5q14.617 14.5 35.5 14.5Q308 853 322.5 838.382q14.5-14.617 14.5-35.5Q337 782 322.382 767.5q-14.617-14.5-35.5-14.5ZM154 217h651q16 0 25.5 9.5t9.5 25.813V521q0 17.425-9.5 29.213Q821 562 805 562H154q-15 0-24.5-11.787Q120 538.425 120 521V252.313q0-16.313 9.5-25.813T154 217Zm26 60v225h600V277H180Zm-26 353h647q15 0 27 12.5t12 28.527V935q0 20-12 30.5T801 976H159q-16 0-27.5-10.5T120 935V671.027q0-16.027 9.5-28.527T154 630Zm26 60v226h600V690H180Zm0-413v225-225Zm0 413v226-226Z"
						/></svg
					>
				{:else}
					<svg
						xmlns="http://www.w3.org/2000/svg"
						class="h-5 w-5 text-red-400 dark:text-red-600"
						fill="currentColor"
						viewBox="0 96 960 960"
						><path
							d="M286.882 339Q266 339 251.5 353.618q-14.5 14.617-14.5 35.5Q237 410 251.618 424.5q14.617 14.5 35.5 14.5Q308 439 322.5 424.382q14.5-14.617 14.5-35.5Q337 368 322.382 353.5q-14.617-14.5-35.5-14.5Zm0 414Q266 753 251.5 767.618q-14.5 14.617-14.5 35.5Q237 824 251.618 838.5q14.617 14.5 35.5 14.5Q308 853 322.5 838.382q14.5-14.617 14.5-35.5Q337 782 322.382 767.5q-14.617-14.5-35.5-14.5ZM154 217h651q16 0 25.5 9.5t9.5 25.813V521q0 17.425-9.5 29.213Q821 562 805 562H154q-15 0-24.5-11.787Q120 538.425 120 521V252.313q0-16.313 9.5-25.813T154 217Zm26 60v225h600V277H180Zm-26 353h647q15 0 27 12.5t12 28.527V935q0 20-12 30.5T801 976H159q-16 0-27.5-10.5T120 935V671.027q0-16.027 9.5-28.527T154 630Zm26 60v226h600V690H180Zm0-413v225-225Zm0 413v226-226Z"
						/></svg
					>
				{/if}
			</IconButtonDisabled>
			<Tooltip triggeredBy="#start-stop-server" arrow={false}>
				{#if server_running}
					{$t('common.server running')}
				{:else}
					{$t('common.server not running')}
				{/if}
			</Tooltip>
			<IconButton class="text-lg" id="language">
				<svg
					xmlns="http://www.w3.org/2000/svg"
					class="h-5 w-5"
					fill="currentColor"
					viewBox="0 96 960 960"
					><path
						d="M480 976q-84 0-157-31.5T196 859q-54-54-85-127.5T80 574q0-84 31-156.5T196 291q54-54 127-84.5T480 176q84 0 157 30.5T764 291q54 54 85 126.5T880 574q0 84-31 157.5T764 859q-54 54-127 85.5T480 976Zm0-58q35-36 58.5-82.5T577 725H384q14 60 37.5 108t58.5 85Zm-85-12q-25-38-43-82t-30-99H172q38 71 88 111.5T395 906Zm171-1q72-23 129.5-69T788 725H639q-13 54-30.5 98T566 905ZM152 665h159q-3-27-3.5-48.5T307 574q0-25 1-44.5t4-43.5H152q-7 24-9.5 43t-2.5 45q0 26 2.5 46.5T152 665Zm221 0h215q4-31 5-50.5t1-40.5q0-20-1-38.5t-5-49.5H373q-4 31-5 49.5t-1 38.5q0 21 1 40.5t5 50.5Zm275 0h160q7-24 9.5-44.5T820 574q0-26-2.5-45t-9.5-43H649q3 35 4 53.5t1 34.5q0 22-1.5 41.5T648 665Zm-10-239h150q-33-69-90.5-115T565 246q25 37 42.5 80T638 426Zm-254 0h194q-11-53-37-102.5T480 236q-32 27-54 71t-42 119Zm-212 0h151q11-54 28-96.5t43-82.5q-75 19-131 64t-91 115Z"
					/></svg
				>
			</IconButton>
			<Tooltip triggeredBy="#language" arrow={false}>
				{$t('common.languages')}
			</Tooltip>
			<Dropdown bind:open={languageDropdownOpen} placement="top" triggeredBy="#language">
				{#each $locales as value}
					<DropdownItem
						on:click={async () => {
							languageDropdownOpen = false;
							$locale = value;
							await invoke('write_to_server_file', {
								key: 'Language',
								value: value,
								valueType: 'String'
							});
						}}>{$t(`lang.${value}`)}</DropdownItem
					>
				{/each}
			</Dropdown>
		</SidebarGroup>
	</SidebarWrapper>
</Sidebar>
