<script lang="ts">
	import Device from '$lib/Device.svelte';
	import DeviceCategory from '$lib/DeviceCategory.svelte';
	import RobotForm from '$lib/forms/DeviceForms/RobotForm.svelte';
	import PlcForm from '$lib/forms/DeviceForms/PLCForm.svelte';
	import RobotUserNodeForm from '$lib/forms/UserNodeForms/RobotForm.svelte';
	import PlcUserNodeForm from '$lib/forms/UserNodeForms/PLCForm.svelte';
	import { invoke } from '@tauri-apps/api/tauri';
	import { t } from '$lib/translations/translations';
	import { Hr } from 'flowbite-svelte';
	import { info } from 'tauri-plugin-log-api';

	let robots = [];
	let plcs = [];
	let device_promises = invoke('get_devices').then((devices) => {
		robots = JSON.parse(devices)
			['Clients'].filter((device) => device['Type'] == 'Robot')
			.map((robot) => {
				robot['UserNodes'] = robot['UserNodes'] ?? [];
				return robot;
			});
		plcs = JSON.parse(devices)
			['Clients'].filter((device) => device['Type'] == 'PLC')
			.map((plc) => {
				plc['UserNodes'] = plc['UserNodes'] ?? [];
				return plc;
			});
		return devices;
	});

	let _set_form_robot: (_: any) => void;
	$: set_form_robot = (event) => {
		_set_form_robot(robots[event.detail.index - 1]);
	};
	let _set_form_plc: (_: any) => void;
	$: set_form_plc = (event) => {
		_set_form_plc(plcs[event.detail.index - 1]);
	};
</script>

{#await device_promises then _}
	<DeviceCategory
		name={$t('devices.robots')}
		set_form={set_form_robot}
		disabled_remove={robots.length == 0}
		let:hide_add_device
		let:hide_user_node_form
		let:editing
		let:change_checkmark
		let:edit_device
		let:show_user_node_form
		let:current_device_for_user_node
		on:remove={async (group) => {
			for (const index of group.detail) {
				await invoke('remove_device', { device: robots[index - 1]['Name'] });
				info(`Removing robot '${robots[index - 1]['Name']}'`);
				robots.splice(index - 1, 1);
			}
			robots = robots;
		}}
	>
		{#each robots as robot, i}
			<Device
				device={robot}
				{editing}
				value={i + 1}
				user_nodes={robot['UserNodes']}
				device_type="robot"
				on:change={change_checkmark}
				on:edit_device={edit_device}
				on:show_user_node_form={show_user_node_form}
				on:add_user_nodes={(event) => {
					const index = robots.findIndex((_robot) => _robot['Name'] === robot['device_name']);
					robots[index]['UserNodes'].push(...event.detail['user_nodes']);
					robots = robots;
				}}
				on:remove_user_node={(user_node) => {
					info(
						`Removing user node '${
							robot['UserNodes'][user_node.detail.index]['Name']
						}' from robot '${robot['Name']}'`
					);
					robot['UserNodes'].splice(user_node.detail.index, 1);
					robots = robots;
				}}
			/>
			{#if i != robots.length - 1}
				<Hr class="my-3" height="h-px" divClass="pl-4 pr-2" />
			{/if}
		{/each}
		<RobotForm
			slot="DeviceForm"
			on:hide={hide_add_device}
			bind:set_form={_set_form_robot}
			on:add={async (robot) => {
				info(`Adding robot ${robot.detail['Name']}`);
				robots.push(robot.detail);
				robots = robots;
			}}
			on:edit={(robot) => {
				info(`Editing robot '${robot.detail['original_name']}'`);
				const index = robots.findIndex(
					(_robot) => _robot['Name'] === robot.detail['original_name']
				);

				robots[index] = robot.detail;
			}}
		/>
		<RobotUserNodeForm
			device_name={current_device_for_user_node}
			slot="UserNodeForm"
			on:hide={hide_user_node_form}
			on:add={(event) => {
				info(
					`Adding user node '${event.detail['user_node']['Name']}' for robot '${event.detail['device_name']}'`
				);
				const index = robots.findIndex((_robot) => _robot['Name'] === event.detail['device_name']);
				robots[index]['UserNodes'].push(event.detail['user_node']);
				robots = robots;
			}}
		/>
	</DeviceCategory>

	<DeviceCategory
		name={$t('devices.plcs')}
		set_form={set_form_plc}
		disabled_remove={plcs.length == 0}
		let:hide_add_device
		let:hide_user_node_form
		let:editing
		let:change_checkmark
		let:edit_device
		let:show_user_node_form
		let:current_device_for_user_node
		on:remove={async (group) => {
			for (const index of group.detail) {
				await invoke('remove_device', { device: plcs[index - 1]['Name'] });
				info(`Removing plc '${plcs[index - 1]['Name']}'`);
				plcs.splice(index - 1, 1);
			}
			plcs = plcs;
		}}
	>
		{#each plcs as plc, i}
			<Device
				device={plc}
				{editing}
				value={i + 1}
				user_nodes={plc['UserNodes']}
				device_type="plc"
				on:change={change_checkmark}
				on:edit_device={edit_device}
				on:show_user_node_form={show_user_node_form}
				on:add_user_nodes={(event) => {
					const index = plcs.findIndex((_plc) => _plc['Name'] === plc['Name']);
					plcs[index]['UserNodes'].push(...event.detail['user_nodes']);
					plcs = plcs;
				}}
				on:remove_user_node={(user_node) => {
					info(
						`Removing user node '${plc['UserNodes'][user_node.detail.index]['Name']}' from plc '${
							plc['Name']
						}'`
					);
					plc['UserNodes'].splice(user_node.detail.index, 1);
					plcs = plcs;
				}}
			/>
			{#if i != plcs.length - 1}
				<Hr class="my-3" height="h-px" divClass="pl-4 pr-2" />
			{/if}
		{/each}
		<PlcForm
			slot="DeviceForm"
			on:hide={hide_add_device}
			bind:set_form={_set_form_plc}
			on:add={async (plc) => {
				info(`Adding plc ${plc.detail['Name']}`);
				plcs.push(plc.detail);
				plcs = plcs;
			}}
			on:edit={(plc) => {
				info(`Editing plc '${plc.detail['original_name']}'`);
				const index = plcs.findIndex((_plc) => _plc['Name'] === plc.detail['original_name']);
				plcs[index] = plc.detail;
			}}
		/>
		<PlcUserNodeForm
			device_name={current_device_for_user_node}
			slot="UserNodeForm"
			on:hide={hide_user_node_form}
			on:add={(event) => {
				info(
					`Adding user node '${event.detail['user_node']['Name']}' for plc '${event.detail['device_name']}'`
				);
				const index = plcs.findIndex((_plc) => _plc['Name'] === event.detail['device_name']);
				plcs[index]['UserNodes'].push(event.detail['user_node']);
				plcs = plcs;
			}}
		/>
	</DeviceCategory>
{/await}
