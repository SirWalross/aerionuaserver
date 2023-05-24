import shutil
import signal
import subprocess
import sys
import mockup.plc as plc_mock
import mockup.robot as robot_mock
import pytest
from asyncua import Client, Node, ua
import json
import re
import time
import os


url = "opc.tcp://127.0.0.1:5001/"
namespace = "urn:open62541.server.application"


@pytest.fixture(autouse=True)
def server():
    # create clients.json
    shutil.copy("tests/test_clients.json", "tests/clients.json")

    # create server.json
    shutil.copy("tests/test_server.json", "tests/server.json")

    # fill dicts
    plc_mock.fill_dicts()

    # start the clients
    clients = [
        subprocess.Popen(["python3", "tests/mockup/robot.py", "10001"]),
        subprocess.Popen(["python3", "tests/mockup/robot.py", "10002"]),
        subprocess.Popen(["python3", "tests/mockup/robot.py", "10003"]),
        subprocess.Popen(["python3", "tests/mockup/plc.py", "5007"]),
    ]

    time.sleep(1)

    # start the server
    server = subprocess.Popen([os.environ["SERVER_EXECUTABLE"]])

    time.sleep(5) # let the server start

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

    # shutdown clients
    for client in clients:
        if sys.platform == "win32":
            client.send_signal(signal.CTRL_C_EVENT)
        else:
            client.send_signal(signal.SIGINT)
        try:
            client.wait(1)
        except subprocess.TimeoutExpired as _:
            client.terminate()
            client.wait()


@pytest.mark.asyncio
async def test_filewatching() -> None:
    async with Client(url=url, timeout=10) as client:
        nsidx = await client.get_namespace_index(namespace)
        plc = await client.nodes.objects.get_child(f"{nsidx}:R04CPU")
        assert await (await plc.get_child(f"{nsidx}:Production Information")).get_value() == plc_mock.PRODUCTION_INFORMATION

        robot = await client.nodes.objects.get_child(f"{nsidx}:Robot1")
        curr_pos: Node = await (await robot.get_child([f"{nsidx}:Variables", f"{nsidx}:Current Position"])).get_value()
        assert curr_pos == list(
            [float(v) for v in robot_mock.ROBOT_P_CURR.replace(")(", ", ").replace(")", "").replace("(", "").split(", ")]
        )

        # remove robot and add new global label to the plc in clients.json
        with open("tests/clients.json", encoding="utf-8", mode="r") as file:
            clients = json.loads("".join(re.sub("(.*?)(//.*)?", "\\1", row) for row in file))
            clients["Clients"].pop(1)  # remove robot
            clients["Clients"][0]["UserNodes"].append(
                {
                    "Name": "udLabel",
                    "Parent": "Global Label",
                    "Type": "GlobalLabel",
                    "Datatype": "DWord",
                    "ReadCommand": {"Label": "udLabel"},
                    "Writeable": True,
                }
            )

        with open("tests/clients.json", encoding="utf-8", mode="w") as file:
            file.write(json.dumps(clients, indent=4))
            file.flush()
            os.fsync(file)
        print("Changed file")

        # let server reconfigure
        time.sleep(5)

        plc = await client.nodes.objects.get_child(f"{nsidx}:R04CPU")
        assert await (await plc.get_child(f"{nsidx}:Production Information")).get_value() == plc_mock.PRODUCTION_INFORMATION

        global_label: Node = await plc.get_child(f"{nsidx}:Global Label")
        udLabel: Node = await global_label.get_child(f"{nsidx}:udLabel")
        assert await udLabel.get_value() == plc_mock.labels["udLabel"][0]

        with pytest.raises(ua.uatypes.UaStatusCodeError) as exc_info:
            await client.nodes.objects.get_child(f"{nsidx}:Roboter1")

        assert exc_info.value.code == ua.uatypes.status_codes.StatusCodes.BadNoMatch

        # remove plc and add robot
        with open("tests/clients.json", encoding="utf-8", mode="r") as file:
            clients = json.loads("".join(re.sub("(.*?)(//.*)?", "\\1", row) for row in file))
            clients["Clients"].pop(0)  # remove plc
            clients["Clients"].append(  # add robot
                {
                    "Name": "Robot2",
                    "Type": "Robot",
                    "Ip": "127.0.0.1",
                    "Port": 10001,
                }
            )

        with open("tests/clients.json", encoding="utf-8", mode="w") as file:
            file.write(json.dumps(clients, indent=4))
            file.flush()
            os.fsync(file)
        print("Changed file")

        # let server reconfigure
        time.sleep(5)

        robot = await client.nodes.objects.get_child(f"{nsidx}:Robot2")
        motion_device: Node = await robot.get_child([f"{nsidx}:MotionDevices", f"{nsidx}:MotionDevice_1"])
        assert await (await motion_device.get_child(f"{nsidx}:Model")).get_value() == robot_mock.ROBOT_TYPE
        assert await (await motion_device.get_child(f"{nsidx}:Manufacturer")).get_value() == "Mitsubishi Electric"
        assert await (await motion_device.get_child(f"{nsidx}:SerialNumber")).get_value() == robot_mock.ROBOT_SERIAL_NUMBER

        with pytest.raises(ua.uatypes.UaStatusCodeError) as exc_info:
            await client.nodes.objects.get_child(f"{nsidx}:R04CPU")

        assert exc_info.value.code == ua.uatypes.status_codes.StatusCodes.BadNoMatch

        # remove robot
        with open("tests/clients.json", encoding="utf-8", mode="r") as file:
            clients = json.loads("".join(re.sub("(.*?)(//.*)?", "\\1", row) for row in file))
            clients["Clients"].pop(0)  # remove robot

        with open("tests/clients.json", encoding="utf-8", mode="w") as file:
            file.write(json.dumps(clients, indent=4))
            file.flush()
            os.fsync(file)
        print("Changed file")

        # let server reconfigure
        time.sleep(5)

        with pytest.raises(ua.uatypes.UaStatusCodeError) as exc_info:
            await client.nodes.objects.get_child(f"{nsidx}:Robot2")

        assert exc_info.value.code == ua.uatypes.status_codes.StatusCodes.BadNoMatch

        # add three new robots
        with open("tests/clients.json", encoding="utf-8", mode="r") as file:
            clients = json.loads("".join(re.sub("(.*?)(//.*)?", "\\1", row) for row in file))
            clients["Clients"].extend(
                [  # add new robots
                    {
                        "Name": "Robot2",
                        "Type": "Robot",
                        "Ip": "127.0.0.1",
                        "Port": 10001,
                    },
                    {
                        "Name": "Robot3",
                        "Type": "Robot",
                        "Ip": "127.0.0.1",
                        "Port": 10002,
                    },
                    {
                        "Name": "Robot4",
                        "Type": "Robot",
                        "Ip": "127.0.0.1",
                        "Port": 10003,
                    },
                ]
            )

        # large buffer size necessary so file is not flushed during writing
        with open("tests/clients.json", encoding="utf-8", mode="w", buffering=4048576) as file:
            file.write(json.dumps(clients, indent=4))
            file.flush()
            os.fsync(file)
        print("Changed file")

        # let server reconfigure
        time.sleep(5)

        robot = await client.nodes.objects.get_child(f"{nsidx}:Robot2")
        motion_device: Node = await robot.get_child([f"{nsidx}:MotionDevices", f"{nsidx}:MotionDevice_1"])
        assert await (await motion_device.get_child(f"{nsidx}:Model")).get_value() == robot_mock.ROBOT_TYPE
        assert await (await motion_device.get_child(f"{nsidx}:Manufacturer")).get_value() == "Mitsubishi Electric"
        assert await (await motion_device.get_child(f"{nsidx}:SerialNumber")).get_value() == robot_mock.ROBOT_SERIAL_NUMBER

        robot = await client.nodes.objects.get_child(f"{nsidx}:Robot3")
        motion_device: Node = await robot.get_child([f"{nsidx}:MotionDevices", f"{nsidx}:MotionDevice_1"])
        assert await (await motion_device.get_child(f"{nsidx}:Model")).get_value() == robot_mock.ROBOT_TYPE
        assert await (await motion_device.get_child(f"{nsidx}:Manufacturer")).get_value() == "Mitsubishi Electric"
        assert await (await motion_device.get_child(f"{nsidx}:SerialNumber")).get_value() == robot_mock.ROBOT_SERIAL_NUMBER

        robot = await client.nodes.objects.get_child(f"{nsidx}:Robot4")
        motion_device: Node = await robot.get_child([f"{nsidx}:MotionDevices", f"{nsidx}:MotionDevice_1"])
        assert await (await motion_device.get_child(f"{nsidx}:Model")).get_value() == robot_mock.ROBOT_TYPE
        assert await (await motion_device.get_child(f"{nsidx}:Manufacturer")).get_value() == "Mitsubishi Electric"
        assert await (await motion_device.get_child(f"{nsidx}:SerialNumber")).get_value() == robot_mock.ROBOT_SERIAL_NUMBER
