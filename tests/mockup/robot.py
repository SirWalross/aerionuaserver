"""Mock Robotcontroller implementing the R3 Protocoll"""

import socket
import sys
import traceback

ROBOT_TYPE = "RV-4FRLM-D"
ROBOT_CONTROLLER_TYPE = "CR8xx-D"
ROBOT_CONTROLLER_VERSION = "Ver.C2g"
ROBOT_SERIAL_NUMBER = "1234567"
ROBOT_AXIS_PATTERN = 0x7F  # 6 axis and 1 additional axis
ROBOT_XYZ_PATTERN = 0x7F  # xyzabz and l1
ROBOT_MECHA_PATTERN = 0x3  # robot mech and one additional mecha
ROBOT_JPOS = [
    (0.0, 0.0, 90.0, 0.0, 90.0, 0.0, 90.0),
    (0.0, 90.0, 0.0),
]  # two mechas, one with 7 axes, one with 3
ROBOT_P_CURR = "(10.0, 30.0, 30.0, 0.0, 0.0, 30.0, 0.0, 0)(7, 0)"
ROBOT_P_SAFE = "(10.0, 60.0, 60.0, 0.0, 0.0, 10.0, 0.0, 0)(7, 0)"
ROBOT_OVRD = 50
ROBOT_M_SVO = 1
ROBOT_TEMP = 30
ROBOT_ETEMP = [[4, 5, 6, 7, 8, 9, 10], [11, 12, 13]]
ROBOT_SRVLCR = [[1, 2, 3, 4, 5, 6, 7], [8, 9, 10]]
ROBOT_SRVSPD = [[1, 2, 3, 4, 5, 6, 7], [8, 9, 10]]
ROBOT_TIME = (300, 300)
ROBOT_TASK_SLOT = [("Hallo", "CYC", "START"), *[("", "REP", "START")] * 5, ("Error", "REP", "ERROR"), ("Always", "REP", "ALWAYS")]
ROBOT_IN = 33
ROBOT_OUT = [35] * 5
ROBOT_M_SMARTPLUS = 1
ROBOT_M_MODE = 1
ROBOT_ESTOP = True
ROBOT_ERROR = False
ROBOT_M_ENC = [10, 3000000, -3000, 30000, 10, 10, 0, 0]
ROBOT_MAINTENANCELOG_GREASE = [f"{i+1}" for i in range(10)]
ROBOT_ADDITIONAL_COMPONENTS = ["ETHERNET;TEST", *["None;None"] * 6, "CC-LINK;TEST4"]
ROBOT_M1 = 3


answers = {
    "OPEN=": (
        f"{ROBOT_XYZ_PATTERN:X};{ROBOT_AXIS_PATTERN:X};7,0;3,5,A,1E,32,46,64;MB6;PRM;{ROBOT_TYPE};{ROBOT_CONTROLLER_TYPE};MELFA;22-07-20;{ROBOT_CONTROLLER_VERSION};ENG;COPYRIGHT(C)2017-2022"
        f" MITSUBISHI ELECTRIC CORPORATION ALL RIGHTS RESERVED;{ROBOT_MECHA_PATTERN:X};5;{len(ROBOT_TASK_SLOT)};"
    ),
    **{f"{mecha+1};1;JPOS{i+1}": f"J{i+1};{value}" for mecha, pos in enumerate(ROBOT_JPOS) for i, value in enumerate(pos)},
    **{f"{mecha+1};1;JPOSF": ";".join(f"J{i+1};{value}" for i, value in enumerate(pos)) for mecha, pos in enumerate(ROBOT_JPOS)},
    **{f"{mecha+1};1;ETEMP": ";".join(f"{value}" for value in temps) for mecha, temps in enumerate(ROBOT_ETEMP)},
    **{f"{mecha+1};1;SRVLCR": ";".join(f"{value};{value}" for value in load_ratio) for mecha, load_ratio in enumerate(ROBOT_SRVLCR)},
    **{f"{mecha+1};1;SRVSPD": ";".join(f"{value};{value};{value}" for value in speed) for mecha, speed in enumerate(ROBOT_SRVSPD)},
    "RAREAD": f";{ROBOT_SERIAL_NUMBER}",
    "OVRD": f"{ROBOT_OVRD}",
    "VALM_Svo;M": f"M_Svo=+{ROBOT_M_SVO}",
    "OPNUMRD": f"{len(ROBOT_ADDITIONAL_COMPONENTS)}",
    **{f"OPSTSRD{i+1}": f"{product_type}" for i, product_type in enumerate(ROBOT_ADDITIONAL_COMPONENTS)},
    "PTIME": f"{ROBOT_TIME[0]};{ROBOT_TIME[1]}",
    "THMRD": f"{ROBOT_TEMP}",
    **{f"1;{taskno+1};SLOTRD": f"{name};{mode};{condition};1" for taskno, (name, mode, condition) in enumerate(ROBOT_TASK_SLOT)},
    **{f"1;{taskno+1};PRGRD": f"{name}" for taskno, (name, _, _) in enumerate(ROBOT_TASK_SLOT)},
    "IN": f"{ROBOT_IN:x}",
    "OUT=": "",
    **{f"OUT{i * 16}": f"{val:x}" for i, val in enumerate(ROBOT_OUT)},
    "VALM_SmartPlus(102)": f"M_SmartPlus(102)=+{ROBOT_M_SMARTPLUS}",
    "VALP_Curr": f"P_Curr={ROBOT_P_CURR}",
    "VALP_Safe(1)": f"P_Safe(1)={ROBOT_P_SAFE}",
    "VALM_Mode;M": f"M_Mode=+{ROBOT_M_MODE}",
    "DSTATE": f"000{int(ROBOT_ESTOP)}{int(ROBOT_ERROR)}",
    **{f"VALM_Enc({i+1})": f"M_Enc({i+1})={val}" for i, val in enumerate(ROBOT_M_ENC)},
    **{f"VALC_PMLogGrs({i+1}": f'C_PMLogGrs({i+1},1)="{val}"' for i, val in reversed(list(enumerate(ROBOT_MAINTENANCELOG_GREASE)))},
    "1;3;VALM1": f"M1={ROBOT_M1}",
}


def main() -> None:
    port = 10001 if len(sys.argv) == 1 else int(sys.argv[1])
    r3 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    r3.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    r3.bind(("localhost", port))
    r3.listen(2)
    print(f"Robot mockup running on localhost:{port}")
    while True:
        try:
            (conn, addr) = r3.accept()

            while True:
                message = conn.recv(1024).decode("latin-1")
                if message != "":
                    try:
                        answer = [
                            f"QoK{value}"
                            for key, value in answers.items()
                            if message.lower().startswith(key.lower()) or ";".join(message.split(";")[2:]).lower().startswith(key.lower())
                        ][0]
                        if message.startswith("1;1;OUT="):
                            answers.update({f"OUT{message.split('=')[1].split(';')[0]}": message.split(";")[3]})

                        conn.sendto(answer.encode("latin-1"), addr)
                    except IndexError:
                        print(f"Invalid message: {message}")
                        conn.sendto(b"QeR", addr)
                else:
                    raise ConnectionError()
        except BrokenPipeError:
            print(traceback.format_exc())
        except ConnectionError:
            print(traceback.format_exc())


if __name__ == "__main__":
    main()
