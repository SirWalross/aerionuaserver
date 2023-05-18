<script>
	import '../app.postcss';
	import Navbar from '../lib/Navbar.svelte';
	import '../styles.css';
</script>

<div class="flex flex-row h-full">
	<Navbar />
	<main class="ml-[14rem] mt-[1rem] mr-[2rem] mb-[2rem] w-full h-max">
		<slot />
	</main>
</div>

<style>
	@font-face {
		font-family: 'Noto Sans JP';
		font-style: normal;
		font-weight: 400;
		src: url(NotoSansJP-Regular.otf) format('opentype');
	}

	:global(body) {
		font-family: 'DejaVu Sans', 'Noto Sans JP';
	}
</style>
