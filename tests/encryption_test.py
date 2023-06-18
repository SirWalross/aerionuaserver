import shutil
import subprocess
import signal
import sys
import time
import pytest
from asyncua import Client, ua
from asyncua.crypto.security_policies import SecurityPolicyAes128Sha256RsaOaep
import asyncio
import os


url = "opc.tcp://127.0.0.1:5001/"
namespace = "urn:open62541.server.application"

endpoints = [
    {"mode": ua.MessageSecurityMode.None_, "security_policy_uri": "http://opcfoundation.org/UA/SecurityPolicy#None"},
    {"mode": ua.MessageSecurityMode.Sign, "security_policy_uri": "http://opcfoundation.org/UA/SecurityPolicy#Basic128Rsa15"},
    {"mode": ua.MessageSecurityMode.SignAndEncrypt, "security_policy_uri": "http://opcfoundation.org/UA/SecurityPolicy#Basic128Rsa15"},
    {"mode": ua.MessageSecurityMode.Sign, "security_policy_uri": "http://opcfoundation.org/UA/SecurityPolicy#Basic256"},
    {"mode": ua.MessageSecurityMode.SignAndEncrypt, "security_policy_uri": "http://opcfoundation.org/UA/SecurityPolicy#Basic256"},
    {"mode": ua.MessageSecurityMode.Sign, "security_policy_uri": "http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256"},
    {"mode": ua.MessageSecurityMode.SignAndEncrypt, "security_policy_uri": "http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256"},
    {"mode": ua.MessageSecurityMode.Sign, "security_policy_uri": "http://opcfoundation.org/UA/SecurityPolicy#Aes128_Sha256_RsaOaep"},
    {
        "mode": ua.MessageSecurityMode.SignAndEncrypt,
        "security_policy_uri": "http://opcfoundation.org/UA/SecurityPolicy#Aes128_Sha256_RsaOaep",
    },
]
client_certificate = "tests/certificates/client_cert.der"
private_key = "tests/certificates/client_key.der"


@pytest.fixture()
def server(request):
    # write empty clients.json
    with open("tests/clients.json", encoding="utf-8", mode="w") as file:
        file.write('{"Clients": []}')

    # write server.json
    with open("tests/server.json", encoding="utf-8", mode="w") as file:
        file.write(request.param["server"])

    if request.param["add_to_trustlist"]:
        # add client certificate to trustlist
        shutil.copy(client_certificate, "tests/server/trusted")
    elif os.path.isdir("tests/server/trusted"):
        # remove all certificates from trustlist
        shutil.rmtree("tests/server/trusted")
        os.mkdir("tests/server/trusted")
    else:
        # create empty trustlist
        os.makedirs("tests/server/trusted")

    server = subprocess.Popen([os.environ["SERVER_EXECUTABLE"]], stderr=subprocess.PIPE)

    for line in server.stderr:
        if b"TCP network layer listening on" in line:
            break

    yield

    # print all stderr from server
    print("Server stderr:")
    for line in server.stderr:
        print(line)

    if sys.platform == "win32":
        server.send_signal(signal.CTRL_C_EVENT)
    else:
        server.send_signal(signal.SIGINT)

    try:
        return_code = server.wait(1)

        if return_code:
            raise subprocess.CalledProcessError(
                return_code, [os.environ["SERVER_EXECUTABLE"]]
            )
    except subprocess.TimeoutExpired:
        server.terminate()
        server.wait()


@pytest.mark.asyncio
@pytest.mark.parametrize(
    "server",
    [
        {
            "server": (
                '{"SecurityPolicies": [ "None", "Basic128Rsa15", "Basic256", "Basic256Sha256", "Aes128Sha256RsaOaep" ], '
                '"SecurityModes": [ "Sign", "Sign & Encrypt" ], "Port": 5001}'
            ),
            "add_to_trustlist": False,
        }
    ],
    indirect=True,
)
async def test_encryption_with_none(server) -> None:
    # test connection endpoints
    async with Client(url=url, timeout=10) as client:
        connection_endpoints = await client.get_endpoints()
        for connection_endpoint in connection_endpoints:
            assert [
                endpoint
                for endpoint in endpoints
                if connection_endpoint.SecurityMode == endpoint["mode"]
                and connection_endpoint.SecurityPolicyUri == endpoint["security_policy_uri"]
            ]

    # test connecting without encryption
    async with Client(url=url, timeout=10) as client:
        await client.get_namespace_index(namespace)

    # test connecting with Aes128Sha256RsaOaep, Sign
    client = Client(url=url, timeout=10)
    await client.set_security(
        SecurityPolicyAes128Sha256RsaOaep,
        client_certificate,
        private_key,
        mode=ua.MessageSecurityMode.Sign,
    )
    async with client:
        await client.get_namespace_index(namespace)

    # test connecting with Aes128Sha256RsaOaep, Sign & Encryption
    client = Client(url=url, timeout=10)
    await client.set_security(
        SecurityPolicyAes128Sha256RsaOaep,
        client_certificate,
        private_key,
        mode=ua.MessageSecurityMode.SignAndEncrypt,
    )
    async with client:
        await client.get_namespace_index(namespace)


@pytest.mark.asyncio
@pytest.mark.parametrize(
    "server",
    [
        {
            "server": '{"SecurityPolicies": [ "Aes128Sha256RsaOaep" ], "SecurityModes": [ "Sign & Encrypt" ], "Port": 5001}',
            "add_to_trustlist": False,
        }
    ],
    indirect=True,
)
async def test_encryption_without_none(server) -> None:
    # only include Aes128Sha256RsaOaep and Sign & Encryption

    # test connecting without encryption (should fail)
    with pytest.raises(ua.uatypes.UaStatusCodeError) as exc_info:
        async with Client(url=url, timeout=10) as client:
            await client.get_namespace_index(namespace)

    assert exc_info.value.code == ua.uatypes.status_codes.StatusCodes.BadSecurityPolicyRejected

    # test connecting with Aes128Sha256RsaOaep, Sign (should fail)
    client = Client(url=url)
    with pytest.raises(ua.uatypes.UaError) as exc_info:
        await client.set_security(
            SecurityPolicyAes128Sha256RsaOaep,
            client_certificate,
            private_key,
            mode=ua.MessageSecurityMode.Sign,
        )
        async with client:
            await client.get_namespace_index(namespace)

    assert "no matching endpoints" in exc_info.value.args[0].lower()

    # test connecting with Aes128Sha256RsaOaep, Sign & Encryption
    client = Client(url=url)
    await client.set_security(
        SecurityPolicyAes128Sha256RsaOaep,
        client_certificate,
        private_key,
        mode=ua.MessageSecurityMode.SignAndEncrypt,
    )
    async with client:
        await client.get_namespace_index(namespace)


@pytest.mark.asyncio
@pytest.mark.parametrize(
    "server",
    [
        {
            "server": (
                '{"SecurityPolicies": [ "Aes128Sha256RsaOaep" ], "SecurityModes": [ "Sign & Encrypt" ], '
                '"EnforceClientCertificateVerification": true, "Port": 5001}'
            ),
            "add_to_trustlist": False,
        }
    ],
    indirect=True,
)
async def test_client_certificate_verification(server) -> None:
    # enforce client certificate verification

    # test connecting without adding to client trustlist (should fail)
    client = Client(url=url)
    await client.set_security(
        SecurityPolicyAes128Sha256RsaOaep,
        client_certificate,
        private_key,
        mode=ua.MessageSecurityMode.SignAndEncrypt,
    )
    # TODO: better check if BadCertificateUntrusted is really the issue
    with pytest.raises(asyncio.TimeoutError):
        async with client:
            await client.get_namespace_index(namespace)


@pytest.mark.asyncio
@pytest.mark.parametrize(
    "server",
    [
        {
            "server": (
                '{"SecurityPolicies": [ "Aes128Sha256RsaOaep" ], "SecurityModes": [ "Sign & Encrypt" ], '
                '"EnforceClientCertificateVerification": true, "Port": 5001}'
            ),
            "add_to_trustlist": True,
        }
    ],
    indirect=True,
)
async def test_client_certificate_verification_add_to_trustlist(server) -> None:
    # enforce client certificate verification

    # test connecting with adding to client trustlist
    client = Client(url=url)
    print(client.application_uri)
    await client.set_security(
        SecurityPolicyAes128Sha256RsaOaep,
        client_certificate,
        private_key,
        mode=ua.MessageSecurityMode.SignAndEncrypt,
    )
    async with client:
        await client.get_namespace_index(namespace)
