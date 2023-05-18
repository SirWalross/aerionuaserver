import shutil
import signal
import subprocess
import sys
import time
from typing import AsyncGenerator, Tuple
import mockup.robot as robot_mock
import pytest
from asyncua import Client, Node, ua
import asyncio
import os


url = "opc.tcp://127.0.0.1:5001/"
namespace = "urn:open62541.server.application"


@pytest.fixture(autouse=True)
def server():
    # create clients.json
    shutil.copy("tests/test_clients.json", "tests/clients.json")

    # create server.json
    shutil.copy("tests/test_server.json", "tests/server.json")

    # start the client
    client = subprocess.Popen(["python3", "tests/mockup/robot.py"])

    time.sleep(1)

    # start the server
    server = subprocess.Popen([os.environ["SERVER_EXECUTABLE"]])

    time.sleep(1)  # let the server start

    yield

    # shutdown server
    if sys.platform == "win32":
        server.send_signal(signal.CTRL_C_EVENT)
    else:
        server.send_signal(signal.SIGINT)

    try:
        server.wait(1)
    except subprocess.TimeoutExpired as _:
        server.terminate()
        server.wait()

    # shutdown client
    client.send_signal(signal.SIGINT)
    try:
        client.wait(1)
    except subprocess.TimeoutExpired as _:
        client.terminate()
        client.wait()


@pytest.fixture
async def robot_node() -> AsyncGenerator[Tuple[Node, int], None]:
    async with Client(url=url) as client:
        nsidx = await client.get_namespace_index(namespace)
        yield (await client.nodes.objects.get_child(f"{nsidx}:Robot1")), nsidx


@pytest.mark.asyncio
async def test_io(robot_node: AsyncGenerator[Tuple[Node, int], None]):
    (robot, nsidx) = await robot_node.__anext__()
    robot_input: Node = await robot.get_child([f"{nsidx}:Inputs", f"{nsidx}:Inputs_0_15"])
    assert (await robot_input.get_value()) == robot_mock.ROBOT_IN
    robot_output: Node = await robot.get_child([f"{nsidx}:Outputs", f"{nsidx}:Outputs_0_15"])
    assert (await robot_output.get_value()) == robot_mock.ROBOT_OUT[0]
    await robot_output.write_value(10, ua.VariantType.Int32)
    assert (await robot_output.get_value()) == 10


@pytest.mark.asyncio
async def test_user_nodes(robot_node: AsyncGenerator[Tuple[Node, int], None]):
    (robot, nsidx) = await robot_node.__anext__()
    curr_pos: Node = await (await robot.get_child([f"{nsidx}:Variables", f"{nsidx}:Current Position"])).get_value()
    assert curr_pos == list([float(v) for v in robot_mock.ROBOT_P_CURR.replace(")(", ", ").replace(")", "").replace("(", "").split(", ")])
    safe_pos: Node = await (
        await robot.get_child([f"{nsidx}:MotionDevices", f"{nsidx}:MotionDevice_1", f"{nsidx}:ParameterSet", f"{nsidx}:Safe Position"])
    ).get_value()
    assert safe_pos == list([float(v) for v in robot_mock.ROBOT_P_SAFE.replace(")(", ", ").replace(")", "").replace("(", "").split(", ")])

    robot_output: Node = await robot.get_child([f"{nsidx}:Outputs", f"{nsidx}:Outputs_64_79"])
    assert (await robot_output.get_value()) == robot_mock.ROBOT_OUT[4]
    await robot_output.write_value(10, ua.VariantType.Int32)
    assert (await robot_output.get_value()) == 10

    encoder_values = await (await robot.get_child([f"{nsidx}:Variables", f"{nsidx}:Encoder Values"])).get_value()
    assert encoder_values == robot_mock.ROBOT_M_ENC

    local_variable = await (await robot.get_child([f"{nsidx}:Variables", f"{nsidx}:LocalVariable"])).get_value()
    assert local_variable == robot_mock.ROBOT_M1


@pytest.mark.asyncio
async def test_controller(robot_node: AsyncGenerator[Tuple[Node, int], None]):
    (robot, nsidx) = await robot_node.__anext__()
    controller: Node = await robot.get_child([f"{nsidx}:Controllers", f"{nsidx}:Controller_1"])
    assert await (await controller.get_child(f"{nsidx}:Model")).get_value() == robot_mock.ROBOT_CONTROLLER_TYPE
    assert (
        await (
            await controller.get_child(
                [
                    f"{nsidx}:Software",
                    f"{nsidx}:MainSoftware",
                    f"{nsidx}:SoftwareRevision",
                ]
            )
        ).get_value()
        == robot_mock.ROBOT_CONTROLLER_VERSION
    )

    assert len(await (await controller.get_child(f"{nsidx}:TaskControls")).get_children()) == len(robot_mock.ROBOT_TASK_SLOT)
    for i, (name, mode, condition) in enumerate(robot_mock.ROBOT_TASK_SLOT):
        task_control: Node = await controller.get_child(
            [
                f"{nsidx}:TaskControls",
                f"{nsidx}:TaskControl_{i+1}",
            ]
        )

        assert await (await task_control.get_child(f"{nsidx}:ComponentName")).get_value() == f"Slot_{i+1}"
        assert await (await task_control.get_child([f"{nsidx}:ParameterSet", f"{nsidx}:TaskProgramName"])).get_value() == name
        assert await (await task_control.get_child([f"{nsidx}:ParameterSet", f"{nsidx}:TaskProgramLoaded"])).get_value() == (name != "")
        assert (await (await task_control.get_child([f"{nsidx}:ParameterSet", f"{nsidx}:ExecutionMode"])).get_value()).DisplayName.Text == (
            "CYCLE" if mode == "CYC" else "CONTINUOUS"
        )
        assert (
            await (await task_control.get_child([f"{nsidx}:ParameterSet", f"{nsidx}:ExecutionCondition"])).get_value()
        ).DisplayName.Text == condition


@pytest.mark.asyncio
async def test_motion_devices(robot_node: AsyncGenerator[Tuple[Node, int], None]):
    (robot, nsidx) = await robot_node.__anext__()
    motion_devices: Node = await (await robot.get_child(f"{nsidx}:MotionDevices")).get_children()

    assert len(motion_devices) == bin(robot_mock.ROBOT_MECHA_PATTERN).count("1")

    for i, _ in enumerate(motion_devices):
        motion_device = await robot.get_child([f"{nsidx}:MotionDevices", f"{nsidx}:MotionDevice_{i+1}"])
        assert await (await motion_device.get_child(f"{nsidx}:Model")).get_value() == (robot_mock.ROBOT_TYPE if i == 0 else "USER")
        assert await (await motion_device.get_child(f"{nsidx}:Manufacturer")).get_value() == "Mitsubishi Electric"
        assert await (await motion_device.get_child(f"{nsidx}:SerialNumber")).get_value() == robot_mock.ROBOT_SERIAL_NUMBER

        assert (await (await motion_device.get_child(f"{nsidx}:MotionDeviceCategory")).get_value()).DisplayName.Text == (
            ("ARTICULATED_ROBOT" if robot_mock.ROBOT_TYPE.lower().startswith("rv") else "SCARA_ROBOT") if i == 0 else "OTHER"
        )

        assert (
            await (await motion_device.get_child([f"{nsidx}:ParameterSet", f"{nsidx}:SpeedOverride"])).get_value() == robot_mock.ROBOT_OVRD
        )
        assert await (await motion_device.get_child([f"{nsidx}:ParameterSet", f"{nsidx}:InControl"])).get_value() == robot_mock.ROBOT_M_SVO

        assert len(await (await motion_device.get_child(f"{nsidx}:Axes")).get_children()) == len(robot_mock.ROBOT_JPOS[i])

        if i == 0:
            joint = await motion_device.get_child([f"{nsidx}:Axes", f"{nsidx}:Axis_J3"])
            assert (await (await joint.get_child(f"{nsidx}:MotionProfile")).get_value()).DisplayName.Text == (
                "ROTARY" if robot_mock.ROBOT_TYPE.lower().startswith("rv") else "LINEAR"
            )

        for j, _ in enumerate(robot_mock.ROBOT_JPOS):
            joint = await motion_device.get_child([f"{nsidx}:Axes", f"{nsidx}:Axis_J{j+1}"])
            assert (
                await (await joint.get_child([f"{nsidx}:ParameterSet", f"{nsidx}:ActualPosition"])).get_value()
                == robot_mock.ROBOT_JPOS[i][j]
            )
            assert (
                await (await joint.get_child([f"{nsidx}:ParameterSet", f"{nsidx}:ActualSpeed"])).get_value()
                == robot_mock.ROBOT_SRVSPD[i][j]
            )

        assert len(await (await motion_device.get_child(f"{nsidx}:PowerTrains")).get_children()) == len(robot_mock.ROBOT_JPOS[i])

        for j, _ in enumerate(robot_mock.ROBOT_ETEMP):
            motor_identifier = await motion_device.get_child(
                [f"{nsidx}:PowerTrains", f"{nsidx}:PowerTrain_{j+1}", f"{nsidx}:MotorIdentifier"]
            )
            assert (
                await (await motor_identifier.get_child([f"{nsidx}:ParameterSet", f"{nsidx}:MotorTemperature"])).get_value()
                == robot_mock.ROBOT_ETEMP[i][j]
            )
            assert (
                await (await motor_identifier.get_child([f"{nsidx}:ParameterSet", f"{nsidx}:EffectiveLoadRatio"])).get_value()
                == robot_mock.ROBOT_SRVLCR[i][j]
            )

        if i == 0:
            additional_components = await (await motion_device.get_child(f"{nsidx}:AdditionalComponents")).get_children()
            assert len(additional_components) == len(
                [
                    additional_component
                    for additional_component in robot_mock.ROBOT_ADDITIONAL_COMPONENTS
                    if (additional_component is not None)
                ]
            )

            additional_component = await motion_device.get_child([f"{nsidx}:AdditionalComponents", f"{nsidx}:Slot_1"])

            assert (
                await (await additional_component.get_child(f"{nsidx}:ProductType")).get_value()
                == robot_mock.ROBOT_ADDITIONAL_COMPONENTS[0].split(";")[0]
            )
            assert (
                await (await additional_component.get_child(f"{nsidx}:Description")).get_value()
                == robot_mock.ROBOT_ADDITIONAL_COMPONENTS[0].split(";")[1]
            )
        else:
            # only first mecha should have additional components
            with pytest.raises(ua.uatypes.UaStatusCodeError) as exc_info:
                await motion_device.get_child([f"{nsidx}:AdditionalComponents"])

            assert exc_info.value.code == ua.uatypes.status_codes.StatusCodes.BadNoMatch


@pytest.mark.asyncio
async def test_safety_state(robot_node: AsyncGenerator[Tuple[Node, int], None]):
    (robot, nsidx) = await robot_node.__anext__()
    safety_state_param: Node = await robot.get_child([f"{nsidx}:SafetyStates", f"{nsidx}:SafetyState_1", f"{nsidx}:ParameterSet"])
    assert await (await safety_state_param.get_child(f"{nsidx}:EmergencyStop")).get_value() == robot_mock.ROBOT_ESTOP
    assert await (await safety_state_param.get_child(f"{nsidx}:ProtectiveStop")).get_value() == robot_mock.ROBOT_ERROR
    assert (await (await safety_state_param.get_child(f"{nsidx}:OperationalMode")).get_value()).DisplayName.Text == (
        "AUTOMATIC" if robot_mock.ROBOT_M_MODE == 2 else "MANUAL_REDUCED_SPEED"
    )


@pytest.mark.asyncio
async def test_smart_plus(robot_node: AsyncGenerator[Tuple[Node, int], None]):
    (robot, nsidx) = await robot_node.__anext__()
    maintenance_log_grease_j1 = await (
        await robot.get_child([f"{nsidx}:SmartPlus", f"{nsidx}:MaintenanceLog", f"{nsidx}:MaintenanceLog (grease)", f"{nsidx}:Axis 3"])
    ).get_value()
    assert robot_mock.ROBOT_MAINTENANCELOG_GREASE == maintenance_log_grease_j1
