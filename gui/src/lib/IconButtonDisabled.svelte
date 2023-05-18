<script lang="ts">
</script>

<button
	aria-label="button"
	type="button"
	{...$$restProps}
	on:change
	on:keydown
	on:keyup
	on:mouseenter
	on:mouseleave
	class="text-gray-500 dark:text-gray-400 hover:bg-gray-100 dark:hover:bg-gray-700 rounded-lg text-sm p-2.5"
>
	<slot />
</button>
