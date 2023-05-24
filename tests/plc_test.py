import shutil
import signal
import subprocess
import sys
import time
from typing import AsyncGenerator, Tuple
import mockup.plc as plc_mock
import pytest
from asyncua import Client, Node, ua
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
    client = subprocess.Popen(["python3", "tests/mockup/plc.py"])

    # fill dicts
    plc_mock.fill_dicts()

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
    except subprocess.TimeoutExpired:
        server.terminate()
        server.wait()

    # shutdown client
    if sys.platform == "win32":
        client.send_signal(signal.CTRL_C_EVENT)
    else:
        client.send_signal(signal.SIGINT)
    try:
        client.wait(1)
    except subprocess.TimeoutExpired:
        client.terminate()
        client.wait()


@pytest.fixture()
async def plc_node() -> AsyncGenerator[Tuple[Node, int], None]:
    async with Client(url=url, timeout=10) as client:
        nsidx = await client.get_namespace_index(namespace)
        yield (await client.nodes.objects.get_child(f"{nsidx}:R04CPU")), nsidx


@pytest.mark.asyncio
async def test_global_label(plc_node: AsyncGenerator[Tuple[Node, int], None]):
    (plc, nsidx) = await plc_node.__anext__()
    global_label: Node = await plc.get_child(f"{nsidx}:Global Label")

    for label, (value, datatype, writeable) in plc_mock.labels.items():
        # skip udlabel
        if label == "udLabel":
            continue

        node: Node = await global_label.get_child(f"{nsidx}:{label}")
        if datatype == plc_mock.DatatypeId.String:
            if isinstance(value, list):
                assert [bytes(str, encoding="latin-1") for str in await node.get_value()] == value
            else:
                assert bytes(await node.get_value(), encoding="latin-1") == value
        else:
            assert await node.get_value() == value

        if writeable:
            if datatype == plc_mock.DatatypeId.Bool:
                await node.write_value(not value, ua.VariantType.Boolean)
                assert await node.get_value() == (not value)
            elif datatype == plc_mock.DatatypeId.Double:
                await node.write_value(-300.0, ua.VariantType.Double)
                assert await node.get_value() == -300.0
            elif datatype == plc_mock.DatatypeId.DInt:
                await node.write_value(75, ua.VariantType.Int32)
                assert await node.get_value() == 75
            elif datatype == plc_mock.DatatypeId.String:
                await node.write_value("Hallo2", ua.VariantType.String)
                assert await node.get_value() == "Hallo2"


@pytest.mark.asyncio
async def test_global_variables(plc_node: AsyncGenerator[Tuple[Node, int], None]):
    (plc, nsidx) = await plc_node.__anext__()
    global_variables: Node = await plc.get_child(f"{nsidx}:Global Variables")

    for variable, (value, datatype, writeable) in plc_mock.variable_list.items():
        if variable == "D-Bool-Device":
            value = [bit == "1" for bit in f"{value:016b}"[::-1]]
        elif variable == "D-String-Array-Device":
            value = list(map("".join, zip(*[iter(value.decode())] * 4)))
        print(variable)
        print(value)
        node: Node = await global_variables.get_child(f"{nsidx}:{variable}")
        assert await node.get_value() == value

        if writeable:
            if datatype == plc_mock.DatatypeId.Bool:
                await node.write_value(not value, ua.VariantType.Boolean)
                assert await node.get_value() == (not value)
            elif datatype == plc_mock.DatatypeId.Double:
                await node.write_value(-300.0, ua.VariantType.Double)
                assert await node.get_value() == -300.0
            elif datatype == plc_mock.DatatypeId.Float:
                await node.write_value(-300.0, ua.VariantType.Float)
                assert await node.get_value() == -300.0
            elif datatype == plc_mock.DatatypeId.Word:
                await node.write_value(75, ua.VariantType.UInt16)
                assert await node.get_value() == 75
            elif datatype == plc_mock.DatatypeId.DInt:
                await node.write_value(75, ua.VariantType.Int32)
                assert await node.get_value() == 75
            elif datatype == plc_mock.DatatypeId.String:
                await node.write_value("Hallo2", ua.VariantType.String)
                assert await node.get_value() == "Hallo2"


@pytest.mark.asyncio
async def test_special_areas(plc_node: AsyncGenerator[Tuple[Node, int], None]):
    (plc, nsidx) = await plc_node.__anext__()
    assert await (await plc.get_child(f"{nsidx}:Production Information")).get_value() == plc_mock.PRODUCTION_INFORMATION
    assert await (await plc.get_child(f"{nsidx}:Operating Status")).get_value() == plc_mock.OPERATING_STATUS
    assert await (await plc.get_child(f"{nsidx}:Firmware Version")).get_value() == plc_mock.FIRMWARE_VERSION
