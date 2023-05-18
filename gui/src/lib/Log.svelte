<script lang="ts">
	import {
		Skeleton,
		Toolbar,
		ToolbarButton,
		Button,
		ButtonGroup,
		Alert,
		Tooltip
	} from 'flowbite-svelte';
	import { BaseDirectory, readTextFile, writeTextFile } from '@tauri-apps/api/fs';
	import { watch } from 'tauri-plugin-fs-watch-api';
	import { save } from '@tauri-apps/api/dialog';
	import { metadata } from 'tauri-plugin-fs-extra-api';
	import { trace } from 'tauri-plugin-log-api';
	import { highlight, languages } from 'prismjs';
	import 'prismjs/components/prism-log';
	import { downloadDir } from '@tauri-apps/api/path';
	import { t } from '$lib/translations/translations';

	export let filepath: string;
	export let defaultFilename: string;

	let code_visible: boolean = true;
	let get_filecontents = (filename: string) => {
		return metadata(filename).then((value) => {
			if (value.size > 1e6) {
				return Promise.reject('file-too-big');
			} else if (!value.isFile) {
				return Promise.reject('not-file');
			} else {
				return readTextFile(filepath, { dir: BaseDirectory.AppConfig });
			}
		});
	};
	let filecontents = get_filecontents(filepath);

	watch(filepath, {}, (event) => {
		if (event[0].path == filepath) {
			trace(`Updating log file: '${filepath}'`);
			filecontents = get_filecontents(filepath);
		}
	});
</script>

<div
	class="w-full rounded-lg bg-gray-50 dark:bg-gray-700 text-gray-900 dark:placeholder-gray-400 dark:text-white, border border-gray-200 dark:border-gray-600"
>
	<div class="border-b py-2 px-3 border-gray-200 dark:border-gray-600">
		<Toolbar embedded>
			<ButtonGroup class="mr-2">
				{#if code_visible}
					<Button
						outline
						on:click={() => {
							code_visible = true;
						}}>{$t('logs.code')}</Button
					>
					<Button
						class="border-l-[1px]"
						on:click={() => {
							code_visible = false;
						}}>{$t('logs.raw')}</Button
					>
				{:else}
					<Button
						on:click={() => {
							code_visible = true;
						}}>{$t('logs.code')}</Button
					>
					<Button
						class="border-l-[1px]"
						outline
						on:click={() => {
							code_visible = false;
						}}>{$t('logs.raw')}</Button
					>
				{/if}
			</ButtonGroup>
			{#await filecontents then lines}
				<p class="text-sm text-gray-400 dark:text-gray-500">
					{(lines.match(/\n/g) || '').length + 1}
					{$t('logs.lines')} · {Math.round(lines.length / 10.0) / 100.0} KB
				</p>
			{/await}
			<ToolbarButton
				name="send"
				slot="end"
				id="download"
				color="dark"
				on:click={async () => {
					const savepath = await save({
						defaultPath: (await downloadDir()) + '/' + defaultFilename,
						filters: [
							{
								name: 'log file (*.log, *.logs)',
								extensions: ['log', 'logs']
							}
						]
					});
					if (savepath) {
						await writeTextFile(savepath, await filecontents);
					}
				}}
				><svg
					xmlns="http://www.w3.org/2000/svg"
					viewBox="0 0 48 48"
					stroke="currentColor"
					fill="currentColor"
					class="w-6 h-6"
					><path
						d="M11 40q-1.2 0-2.1-.9Q8 38.2 8 37v-7.15h3V37h26v-7.15h3V37q0 1.2-.9 2.1-.9.9-2.1.9Zm13-7.65-9.65-9.65 2.15-2.15 6 6V8h3v18.55l6-6 2.15 2.15Z"
					/></svg
				></ToolbarButton
			>
			<Tooltip triggeredBy="#download" arrow={false}>{$t('logs.download log file')}</Tooltip>
		</Toolbar>
	</div>
	<div
		class="py-2 px-4 bg-white dark:bg-gray-800 rounded-b-lg text-sm whitespace-pre-wrap {code_visible
			? 'codedisplay'
			: ''}"
	>
		{#await filecontents}
			<Skeleton />
		{:then lines}
			{#each lines.split('\n') as line}
				{#if code_visible}
					<code
						class="text-gray-700 dark:text-gray-300 before:text-gray-400 before:dark:text-gray-600"
						>{@html highlight(line.replace('\n', '').replace('\r', ''), languages['log'])}<br
						/></code
					>
				{:else}
					<code
						class="text-gray-700 dark:text-gray-300 before:text-gray-400 before:dark:text-gray-600"
						>{line}<br /></code
					>
				{/if}
			{/each}
		{:catch error}
			<Alert border color="red">
				<span slot="icon"
					><svg
						aria-hidden="true"
						class="w-5 h-5"
						fill="currentColor"
						viewBox="0 0 48 48"
						xmlns="http://www.w3.org/2000/svg"
						><path
							d="M24 34q.7 0 1.175-.475.475-.475.475-1.175 0-.7-.475-1.175Q24.7 30.7 24 30.7q-.7 0-1.175.475-.475.475-.475 1.175 0 .7.475 1.175Q23.3 34 24 34Zm-1.35-7.65h3V13.7h-3ZM24 44q-4.1 0-7.75-1.575-3.65-1.575-6.375-4.3-2.725-2.725-4.3-6.375Q4 28.1 4 23.95q0-4.1 1.575-7.75 1.575-3.65 4.3-6.35 2.725-2.7 6.375-4.275Q19.9 4 24.05 4q4.1 0 7.75 1.575 3.65 1.575 6.35 4.275 2.7 2.7 4.275 6.35Q44 19.85 44 24q0 4.1-1.575 7.75-1.575 3.65-4.275 6.375t-6.35 4.3Q28.15 44 24 44Zm.05-3q7.05 0 12-4.975T41 23.95q0-7.05-4.95-12T24 7q-7.05 0-12.025 4.95Q7 16.9 7 24q0 7.05 4.975 12.025Q16.95 41 24.05 41ZM24 24Z"
						/></svg
					></span
				>
				{#if error == 'file-too-big'}
					<span class="font-medium">Error!</span> File '{filepath}' is too big.
				{:else if error == 'not-file'}
					<span class="font-medium">Error!</span> Path '{filepath}' is not a file.
				{:else}
					<span class="font-medium">Error!</span> Could not read file '{filepath}'.
				{/if}
			</Alert>
		{/await}
	</div>
</div>

<svelte:head>
	<link href="/themes/prism-dark.min.css" rel="stylesheet" />
</svelte:head>

<style>
	.codedisplay::before {
		counter-reset: listing;
	}
	.codedisplay code {
		counter-increment: listing;
	}
	.codedisplay code::before {
		content: counter(listing) ' ';
		display: inline-block;
		width: 5ch;
	}
</style>
