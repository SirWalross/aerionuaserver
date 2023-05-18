from enum import Enum
import math
import struct
import socket
import traceback
from typing import Any, Dict, Iterable, List, Optional, Tuple, Union


class Endcode(Enum):
    Success = 0x0000
    InvalidEndCode = 0x0001
    InvalidDevice = 0x0002
    UnableToWrite = 0x0055
    WrongCommand = 0xC059
    WrongFormat = 0xC05C
    WrongLength = 0xC061
    Busy = 0xCEE0
    ExceedReqLength = 0xCEE1
    ExceedRespLength = 0xCEE2
    ServerNotFound = 0xCF10
    WrongConfigItem = 0xCF20
    PrmIDNotFound = 0xCF30
    NotStartExclusiveWrite = 0xCF31
    RelayFailure = 0xCF70
    TimeoutError = 0xCF71
    CANAppNotPermittedRead = 0xCCC7
    CANAppWriteOnly = 0xCCC8
    CANAppReadOnly = 0xCCC9
    CANAppUndefinedObjectAccess = 0xCCCA
    CANAppNotPermittedPDOMapping = 0xCCCB
    CANAppExceedPDOMapping = 0xCCCC
    CANAppNotExistSubIndex = 0xCCD3
    CANAppWrongParameter = 0xCCD4
    CANAppMoreOverParameterRange = 0xCCD5
    CANAppLessOverParameterRange = 0xCCD6
    CANAppTransOrStoreError = 0xCCDA
    CANAppOtherError = 0xCCFF
    OtherNetworkError = 0xCF00
    DataFragmentShortage = 0xCF40
    DataFragmentDup = 0xCF41
    DataFragmentLost = 0xCF43
    DataFragmentNotSupport = 0xCF44
    InvalidGlobalLabel = 0x40C0


class DeviceType(Enum):
    Bit = 0
    Word = 1


class DatatypeId(Enum):
    Bool = ("H", 0x01, 2)
    Word = ("H", 0x02, 2)
    DoubleWord = ("I", 0x03, 4)
    Int = ("h", 0x04, 2)
    DInt = ("i", 0x05, 4)
    Float = ("f", 0x06, 4)
    Double = ("d", 0x07, 8)
    String = ("", 0x09, 1)


class ExtensionSpecification(Enum):
    U3E0 = 0x03E0
    U3E1 = 0x03E1
    U3E2 = 0x03E2
    U3E3 = 0x03E3


class SLMPError(Exception):
    def __init__(self, serial_number: List[int], endcode: Endcode):
        super().__init__(endcode.name)
        self.serial_number = serial_number
        self.endcode = endcode


def pairwise(iterable: Iterable[Any]) -> Iterable[Any]:
    a = iter(iterable)
    return zip(a, a)


FIRMWARE_VERSION = 0xACF3
OPERATING_STATUS = 0x03
PRODUCTION_INFORMATION = "123456789ABCDEFG"
M_DEVICE = "M-Device", [1, 65535, 2, 65535], DatatypeId.Word, False, 96
M_BOOL_DEVICE = "M-Bool-Device", True, DatatypeId.Bool, True, 165
M_BOOL_ARRAY_DEVICE = "M-Bool-Array-Device", [False, True, False, True, False], DatatypeId.Bool, False, 195
M_BOOL_LONG_ARRAY_DEVICE = (
    "M-Bool-Long-Array-Device",
    [False, False, False, False, False, False, True, False, False, False, False, True, False, True, False, False, True, False, True, False],
    DatatypeId.Bool,
    False,
    232,
)
D_DEVICE = "D-Device", 65530, DatatypeId.Word, True, 100
D_BOOL_DEVICE = "D-Bool-Device", 0xABCD, DatatypeId.Bool, False, 101
D_DOUBLE_DEVICE = "D-Double-Device", -5.1349230494293842315673828125e9, DatatypeId.Double, True, 102
D_DINT_DEVICE = "D-DInt-Device", -2147483648, DatatypeId.DInt, True, 106
D_DOUBLE_ARRAY_DEVICE = "D-Double-Array-Device", [1.0, -3.14159265350000005412312020781, 20.0, 10.0e10], DatatypeId.Double, False, 108
D_STRING_ARRAY_DEVICE = "D-String-Array-Device", b"1234234534564567", DatatypeId.String, False, 124  # count = 4, length = 4
U3E0_DEVICE = "U3E0-Device", 30, DatatypeId.Word, True, 100
U3E3_INT_DEVICE = "U3E3-Int-Device", [-3, 5, -7, 9], DatatypeId.Int, False, 101
G_DEVICE = "G-Device", [1, -1, 1, -1], DatatypeId.Int, False, 100
G_FLOAT_DEVICE = "G-Float-Device", -3.1415927410125732421875, DatatypeId.Float, True, 100
B_LABEL = "bLabel", False, DatatypeId.Bool, True
B_ARRAY_LABEL = "bArrayLabel", [False, True, False, True], DatatypeId.Bool, False
E_LABEL = "eLabel", 5.0, DatatypeId.Double, True
S_LABEL = "sLabel", b"Hallo", DatatypeId.String, True
S_ARRAY_LABEL = "sArrayLabel", [b"Hallo", b"Hallo2", b"Hallo3"], DatatypeId.String, False
U_LABEL = "uLabel", [1, 3, 5, 7], DatatypeId.Word, False
WD_LABEL = "wdLabel", -31012121, DatatypeId.DInt, True
UD_LABEL = "udLabel", 31012121, DatatypeId.DoubleWord, True

devices: Dict[str, Dict[int, int]] = {
    "SD": {
        160: FIRMWARE_VERSION,
        **{164 + i: (ord(left) + (ord(right) << 8)) for i, (left, right) in enumerate(pairwise(PRODUCTION_INFORMATION))},
        203: OPERATING_STATUS,
    }
}

variable_list: Dict[str, Tuple[Any, DatatypeId, bool]] = {}

device_map: Dict[int, Tuple[str, DeviceType]] = {
    0x00: ("None", DeviceType.Bit),
    0x91: ("SM", DeviceType.Bit),
    0xA9: ("SD", DeviceType.Word),
    0x9C: ("X", DeviceType.Bit),
    0x9D: ("Y", DeviceType.Bit),
    0x90: ("M", DeviceType.Bit),
    0x92: ("L", DeviceType.Bit),
    0x93: ("F", DeviceType.Bit),
    0x94: ("V", DeviceType.Bit),
    0xA0: ("B", DeviceType.Bit),
    0xA8: ("D", DeviceType.Word),
    0xB4: ("W", DeviceType.Word),
    0xC1: ("TS", DeviceType.Bit),
    0xC0: ("TC", DeviceType.Bit),
    0xC2: ("TN", DeviceType.Word),
    0xC7: ("STS", DeviceType.Bit),
    0xC6: ("STC", DeviceType.Bit),
    0xC8: ("STN", DeviceType.Word),
    0xC4: ("CS", DeviceType.Bit),
    0xC3: ("CC", DeviceType.Bit),
    0xC5: ("CN", DeviceType.Word),
    0xA1: ("SB", DeviceType.Bit),
    0xB5: ("SW", DeviceType.Word),
    0xA2: ("DX", DeviceType.Bit),
    0xA3: ("DY", DeviceType.Bit),
    0xCC: ("Z", DeviceType.Word),
    0xAF: ("R", DeviceType.Word),
    0xB0: ("ZR", DeviceType.Word),
    0x00AB: ("G", DeviceType.Word),
    0x002E: ("HG", DeviceType.Word),
}

labels: Dict[str, Tuple[Any, DatatypeId, bool]] = {}


def response_header(serial_number: List[int]) -> List[int]:
    return [*serial_number, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]


def extension_write_request(command: int, subcommand: int, serial_number: List[int], data: bytes) -> bytes:
    if command == 0x1401 and subcommand == 0x0082:
        try:
            device = device_map[struct.unpack("H", data[6:8])[0]]
            extension_specification = struct.unpack("H", data[10:12])[0]
            head_device_no = struct.unpack("I", data[2:6])[0]
            no_of_devices = struct.unpack("H", data[13:15])[0]
            extension_type = struct.unpack("B", data[12:13])[0]
            print(f"Device write: {device}, {head_device_no}, {no_of_devices}")
        except IndexError:
            return error_response(serial_number, Endcode.InvalidDevice)
        if extension_type == 0xF8 or extension_type == 0xFA:
            values = struct.unpack("H" * int(len(data[15:]) / 2), data[15:])
            write_values(device, subcommand, head_device_no, values, extension_specification, extension_type)
    else:
        return error_response(serial_number, Endcode.WrongCommand)
    return success_response(serial_number)


def write_request(command: int, subcommand: int, serial_number: List[int], data: bytes) -> bytes:
    if command == 0x1401:
        try:
            device = device_map[struct.unpack("B", data[3:4])[0]]
            head_device_no = struct.unpack("I", data[:3] + b"\x00")[0]
            no_of_devices = struct.unpack("H", data[4:6])[0]
        except IndexError:
            return error_response(serial_number, Endcode.InvalidDevice)
        if subcommand == 0x0001 and device[1] == DeviceType.Bit:
            # bit units
            data = struct.unpack("B" * len(data[6:]), data[6:])
            values = [value & (1 << (4 - i * 4)) for value in data for i in range(2)][:no_of_devices]
            write_values(device, subcommand, head_device_no, values)
        elif subcommand == 0x0000 and device[1] == DeviceType.Bit:
            # bit device in word units
            values = struct.unpack("H" * int(len(data[6:]) / 2), data[6:])
            write_values(device, subcommand, head_device_no, values, step=16)
        elif subcommand == 0x0000 and device[1] == DeviceType.Word:
            # word device in word units
            values = struct.unpack("H" * int(len(data[6:]) / 2), data[6:])
            write_values(device, subcommand, head_device_no, values)
        else:
            return error_response(serial_number, Endcode.WrongCommand)
        return success_response(serial_number)
    elif command == 0x141B:
        number_of_points = struct.unpack("H", data[:2])[0]
        number_of_abbreviations = struct.unpack("H", data[2:4])[0]
        start = 4
        abbreviations = []
        for _ in range(number_of_abbreviations):
            abbreviation_length = struct.unpack("H", data[start : (start + 2)])[0]
            abbreviations.append("".join(chr(val) for val in struct.unpack("H", data[(start + 2) : (start + 2 + abbreviation_length)])))
            start += 2 + abbreviation_length
        for _ in range(number_of_points):
            label_name_length = struct.unpack("H", data[start : (start + 2)])[0] * 2
            label_name = "".join(
                chr(val)
                for val in struct.unpack(
                    "H" * int(label_name_length / 2),
                    data[(start + 2) : (start + 2 + label_name_length)],
                )
            )
            label_name, label_index = label_name.split("[")[0], (int(label_name.split("[")[1].split("]")[0]) if "[" in label_name else -1)
            for i, abbreviation in enumerate(abbreviations):
                label_name = label_name.replace(f"%{i + 1}", abbreviation)
            write_data_length = struct.unpack(
                "H",
                data[(start + 2 + label_name_length) : (start + 4 + label_name_length)],
            )[0]
            if labels[label_name][1] == DatatypeId.String:
                write_data = data[(start + 4 + label_name_length) : (start + 4 + label_name_length + write_data_length)]
            else:
                write_data = struct.unpack(
                    labels[label_name][1].value[0],
                    data[(start + 4 + label_name_length) : (start + 4 + label_name_length + write_data_length)],
                )[0]
            if label_index != -1:
                # array label
                labels[label_name][0][label_index] = write_data
            else:
                labels[label_name] = write_data, labels[label_name][1], True
            start += 4 + label_name_length + write_data_length
        return success_response(serial_number)
    else:
        return error_response(serial_number, Endcode.WrongCommand)


def combine_bits(values: List[int], device_type: DeviceType) -> int:
    if device_type == DeviceType.Bit:
        values.extend([0] * 3)  # fill up with zeros
        return (values[3] << 8) + (values[2] << 12) + values[1] + (values[0] << 4)
    else:
        values.extend([0] * 15)  # fill up with zeros
        return sum(value << i for i, value in enumerate(values[:16]))


def get_values(
    device: Tuple[str, DeviceType], head_device_no: int, no_of_devices: int, extension_specification: int = -1, extension_type: int = -1
) -> List[int]:
    if device[1] == DeviceType.Word:
        if extension_specification != -1 and extension_type == 0xF8:
            # module access device
            return [devices[f"U{extension_specification:03X}"][head_device_no + key] for key in range(no_of_devices)]
        elif extension_specification != -1 and extension_type == 0xFA:
            # cpu buffer memory access device
            return [devices[ExtensionSpecification(extension_specification).name][head_device_no + key] for key in range(no_of_devices)]
        else:
            # regular device access
            return [devices[device[0]][head_device_no + key] for key in range(no_of_devices)]
    elif device[1] == DeviceType.Bit:
        values: List[int] = []
        for device_no in {math.floor((head_device_no + i) / 16) * 16 for i in range(no_of_devices)}:
            start = (head_device_no % 16) if device_no == math.floor((head_device_no) / 16) * 16 else 0
            end = ((head_device_no + no_of_devices) % 16) if device_no == math.floor((head_device_no + no_of_devices) / 16) * 16 else 16
            values.extend(int(bit) for i, bit in enumerate(f"{devices[device[0]][device_no]:016b}"[::-1]) if i >= start and i < end)
        return values
    return []


def write_values(
    device: Tuple[str, DeviceType],
    subcommand: int,
    head_device_no: int,
    values: List[Union[bool, int]],
    extension_specification: int = -1,
    extension_type: int = -1,
    step: int = 1,
) -> bool:
    if subcommand == 0x0000 or subcommand == 0x0082:
        # word units
        for i, value in enumerate(values):
            if extension_specification != -1 and extension_type == 0xF8:
                # module access device
                devices[f"U{extension_specification:03X}"][head_device_no + (i * step)] = value
            elif extension_specification != -1 and extension_type == 0xFA:
                # cpu buffer memory access device
                devices[ExtensionSpecification(extension_specification).name][head_device_no + (i * step)] = value
            else:
                # regular device access
                devices[device[0]][head_device_no + (i * step)] = value
        return True
    elif subcommand == 0x0001:
        # bit units
        for i, value in enumerate(values):
            if value:
                devices[device[0]][int((head_device_no + i) / 16) * 16] |= 1 << ((i + head_device_no) % 16)
            else:
                devices[device[0]][int((head_device_no + i) / 16) * 16] &= 0xFFFF ^ (1 << ((i + head_device_no) % 16))
        return True
    else:
        return False


def extension_read_request(command: int, subcommand: int, serial_number: List[int], data: bytes) -> bytes:
    if command == 0x0401 and subcommand == 0x0082:
        try:
            device = device_map[struct.unpack("H", data[6:8])[0]]
            extension_specification = struct.unpack("H", data[10:12])[0]
            head_device_no = struct.unpack("I", data[2:6])[0]
            no_of_devices = struct.unpack("H", data[13:15])[0]
            extension_type = struct.unpack("B", data[12:13])[0]
            print(f"Device read: {device}, {head_device_no}, {no_of_devices}")
        except IndexError:
            return error_response(serial_number, Endcode.InvalidDevice)
        if extension_type == 0xF8 or extension_type == 0xFA:
            values = get_values(device, head_device_no, no_of_devices, extension_specification, extension_type)
            return read_response(serial_number, values)
    return error_response(serial_number, Endcode.WrongCommand)


def read_request(command: int, subcommand: int, serial_number: List[int], data: bytes) -> bytes:
    if command == 0x0401 and (subcommand == 0x0000 or subcommand == 0x001):
        # device read
        try:
            device = device_map[struct.unpack("B", data[3:4])[0]]
            head_device_no = struct.unpack("I", data[:3] + b"\x00")[0]
            no_of_devices = struct.unpack("H", data[4:6])[0]
            print(f"Device read: {device}, {head_device_no}, {no_of_devices}")
        except IndexError:
            return error_response(serial_number, Endcode.InvalidDevice)
        if subcommand == 0x0001 and device[1] == DeviceType.Bit:
            # bit units
            values = [
                combine_bits(get_values(device, head_device_no + 4 * i, 4), DeviceType.Bit) for i in range(math.ceil(no_of_devices / 4))
            ]
            return read_response(serial_number, values)
        elif subcommand == 0x0000 and device[1] == DeviceType.Bit:
            # bit device in word units
            values = [combine_bits(get_values(device, head_device_no + 16 * i, 16), DeviceType.Word) for i in range(no_of_devices)]
            return read_response(serial_number, values)
        elif subcommand == 0x0000 and device[1] == DeviceType.Word:
            # word device in word units
            values = get_values(device, head_device_no, no_of_devices)
            return read_response(serial_number, values)
        else:
            return error_response(serial_number, Endcode.WrongCommand)
    elif command == 0x041C and subcommand == 0x0000:
        # label read
        number_of_points = struct.unpack("H", data[:2])[0]
        number_of_abbreviations = struct.unpack("H", data[2:4])[0]
        start = 4
        response_header_len = len(response_header(serial_number))
        label_data = bytearray(struct.pack("B" * response_header_len, *response_header(serial_number)))
        label_data += struct.pack("H", number_of_points)
        abbreviations = []
        for _ in range(number_of_abbreviations):
            abbreviation_length = struct.unpack("H", data[start : (start + 2)])[0]
            abbreviations.append("".join(chr(val) for val in struct.unpack("H", data[(start + 2) : (start + 2 + abbreviation_length)])))
            start += 2 + abbreviation_length
        for _ in range(number_of_points):
            label_name_length = struct.unpack("H", data[start : (start + 2)])[0] * 2
            label_name = "".join(
                chr(val)
                for val in struct.unpack(
                    "H" * int(label_name_length / 2),
                    data[(start + 2) : (start + 2 + label_name_length)],
                )
            )
            label_name, label_index = label_name.split("[")[0], (int(label_name.split("[")[1].split("]")[0]) if "[" in label_name else -1)
            for i, abbreviation in enumerate(abbreviations):
                label_name = label_name.replace(f"%{i + 1}", abbreviation)
            label_data += struct.pack("B", labels[label_name][1].value[1])

            if labels[label_name][1] == DatatypeId.String:
                # still unsure if string_length of character string is padded or not
                if label_index != -1:
                    # label_index != -1 -> label is array
                    string_length = int((len(labels[label_name][0][label_index]) + 2) / 2) * 2
                else:
                    string_length = int((len(labels[label_name][0]) + 2) / 2) * 2
                label_data += struct.pack("H", string_length)
            else:
                label_data += struct.pack("H", labels[label_name][1].value[2])

            if label_index != -1:
                # label_index != -1 -> label is array
                if labels[label_name][1] == DatatypeId.String:
                    label_data += (labels[label_name][0][label_index] + b"\0" * 2)[:string_length]
                else:
                    label_data += struct.pack(labels[label_name][1].value[0], labels[label_name][0][label_index])
            else:
                if labels[label_name][1] == DatatypeId.String:
                    label_data += (labels[label_name][0] + b"\0" * 2)[:string_length]
                else:
                    label_data += struct.pack(labels[label_name][1].value[0], labels[label_name][0])
            start += 2 + label_name_length
        label_data[(response_header_len - 4) : (response_header_len - 2)] = struct.pack("H", len(label_data) - response_header_len + 2)
        return bytes(label_data)
    else:
        return error_response(serial_number, Endcode.WrongCommand)


def read_response(serial_number: List[int], values: List[int]) -> bytes:
    response_header_len = len(response_header(serial_number))
    data = bytearray(struct.pack("B" * response_header_len, *response_header(serial_number)))
    data += struct.pack("H" * len(values), *values)
    data[(response_header_len - 4) : (response_header_len - 2)] = struct.pack("H", len(data) - len(response_header(serial_number)) + 2)
    return bytes(data)


def error_response(serial_number: List[int], endcode: Endcode) -> bytes:
    answer = response_header(serial_number)
    answer[-4:-2] = 2, 0
    answer[-2:] = endcode.value & 0xFF, (endcode.value >> 8) & 0xFF
    return struct.pack("B" * len(answer), *answer)


def success_response(serial_number: List[int]) -> bytes:
    return error_response(serial_number, Endcode.Success)


def loopback_response(serial_number: List[int], loopback_data: List[int]) -> bytes:
    answer = response_header(serial_number)
    response_len = 2 + len(loopback_data)
    answer[-4:-2] = response_len & 0xFF, response_len & 0xFF00
    answer.extend((len(loopback_data) & 0xFF, len(loopback_data) & 0xFF00))
    answer.extend(loopback_data)
    return struct.pack("B" * len(answer), *answer)


def get_head_nos(head_no: int, count: int) -> List[int]:
    return list({int((head_no + i) / 16) * 16 for i in range(int((count + 15) / 16) * 16)})


def get_value(start_head_no: int, head_no: int, array: List[bool]) -> int:
    if start_head_no > head_no or (start_head_no + len(array)) <= head_no:
        return 0
    else:
        return array[head_no - start_head_no]


def fill_dicts() -> None:
    # fill devices and label dicts
    devices["M"] = {
        **{M_DEVICE[4] + i * 16: v for i, v in enumerate(M_DEVICE[1])},
        int(M_BOOL_DEVICE[4] / 16) * 16: int(M_BOOL_DEVICE[1]) << (M_BOOL_DEVICE[4] % 16),
        int(M_BOOL_DEVICE[4] / 16 + 1) * 16: 0,
        **{
            head_no: sum(get_value(M_BOOL_ARRAY_DEVICE[4], head_no + i, M_BOOL_ARRAY_DEVICE[1]) << i for i in range(16))
            for head_no in get_head_nos(M_BOOL_ARRAY_DEVICE[4], len(M_BOOL_ARRAY_DEVICE[1]))
        },
        **{
            head_no: sum(get_value(M_BOOL_LONG_ARRAY_DEVICE[4], head_no + i, M_BOOL_LONG_ARRAY_DEVICE[1]) << i for i in range(16))
            for head_no in get_head_nos(M_BOOL_LONG_ARRAY_DEVICE[4], len(M_BOOL_LONG_ARRAY_DEVICE[1]))
        },
    }
    devices["D"] = {
        D_DEVICE[4]: D_DEVICE[1],
        D_BOOL_DEVICE[4]: D_BOOL_DEVICE[1],
        **{D_DOUBLE_DEVICE[4] + i: struct.unpack("H", bytes(v))[0] for i, v in enumerate(pairwise(struct.pack("d", D_DOUBLE_DEVICE[1])))},
        **{D_DINT_DEVICE[4] + i: struct.unpack("H", bytes(v))[0] for i, v in enumerate(pairwise(struct.pack("i", D_DINT_DEVICE[1])))},
        **{
            D_DOUBLE_ARRAY_DEVICE[4] + i: struct.unpack("H", bytes(v))[0]
            for i, v in enumerate(pairwise(struct.pack("d" * len(D_DOUBLE_ARRAY_DEVICE[1]), *D_DOUBLE_ARRAY_DEVICE[1])))
        },
        **{
            D_STRING_ARRAY_DEVICE[4] + i: struct.unpack("H", bytes(v))[0]
            for i, v in enumerate(pairwise(struct.pack(f"{len(D_STRING_ARRAY_DEVICE[1])}s", D_STRING_ARRAY_DEVICE[1])))
        },
    }
    devices["U3E0"] = {U3E0_DEVICE[4]: U3E0_DEVICE[1]}
    devices["U3E3"] = {
        **{
            U3E3_INT_DEVICE[4] + i: struct.unpack("H", bytes(v))[0]
            for i, v in enumerate(pairwise(struct.pack("h" * len(U3E3_INT_DEVICE[1]), *U3E3_INT_DEVICE[1])))
        }
    }
    devices["U00A"] = {
        **{
            G_DEVICE[4] + i: struct.unpack("H", bytes(v))[0]
            for i, v in enumerate(pairwise(struct.pack("h" * len(G_DEVICE[1]), *G_DEVICE[1])))
        }
    }
    devices["U00F"] = {
        **{G_FLOAT_DEVICE[4] + i: struct.unpack("H", bytes(v))[0] for i, v in enumerate(pairwise(struct.pack("f", G_FLOAT_DEVICE[1])))},
    }

    labels[B_LABEL[0]] = B_LABEL[1:]
    labels[B_ARRAY_LABEL[0]] = B_ARRAY_LABEL[1:]
    labels[E_LABEL[0]] = E_LABEL[1:]
    labels[S_LABEL[0]] = S_LABEL[1:]
    labels[S_ARRAY_LABEL[0]] = S_ARRAY_LABEL[1:]
    labels[U_LABEL[0]] = U_LABEL[1:]
    labels[WD_LABEL[0]] = WD_LABEL[1:]
    labels[UD_LABEL[0]] = UD_LABEL[1:]

    variable_list[M_DEVICE[0]] = M_DEVICE[1:4]
    variable_list[M_BOOL_DEVICE[0]] = M_BOOL_DEVICE[1:4]
    variable_list[M_BOOL_ARRAY_DEVICE[0]] = M_BOOL_ARRAY_DEVICE[1:4]
    variable_list[D_DEVICE[0]] = D_DEVICE[1:4]
    variable_list[D_BOOL_DEVICE[0]] = D_BOOL_DEVICE[1:4]
    variable_list[D_DOUBLE_DEVICE[0]] = D_DOUBLE_DEVICE[1:4]
    variable_list[D_DINT_DEVICE[0]] = D_DINT_DEVICE[1:4]
    variable_list[D_DOUBLE_ARRAY_DEVICE[0]] = D_DOUBLE_ARRAY_DEVICE[1:4]
    variable_list[D_STRING_ARRAY_DEVICE[0]] = D_STRING_ARRAY_DEVICE[1:4]
    variable_list[U3E0_DEVICE[0]] = U3E0_DEVICE[1:4]
    variable_list[U3E3_INT_DEVICE[0]] = U3E3_INT_DEVICE[1:4]
    variable_list[G_DEVICE[0]] = G_DEVICE[1:4]
    variable_list[G_FLOAT_DEVICE[0]] = G_FLOAT_DEVICE[1:4]


def main() -> None:
    slmp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    slmp.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    slmp.bind(("localhost", 5007))
    slmp.listen(2)
    print("PLC mockup running on localhost:5007")

    while True:
        try:
            (conn, addr) = slmp.accept()

            fill_dicts()

            while True:
                message = conn.recv(1024)
                if len(message) == 0:
                    raise SLMPError([0xD0, 0x00], Endcode.WrongCommand)
                subheader = struct.unpack("H", message[:2])[0]
                if subheader == 0x50:
                    serial_number = [0xD0, 0x00]
                elif subheader == 0x54:
                    serial_number = [
                        0xD4,
                        0x00,
                        *struct.unpack("BB", message[2:4]),
                        0x00,
                        0x00,
                    ]
                elif subheader == 0x68:
                    serial_number = [
                        0xE8,
                        0x00,
                        *struct.unpack("BB", message[2:4]),
                        0x00,
                        0x00,
                    ]
                else:
                    raise SLMPError([0xD0, 0x00], Endcode.WrongFormat)
                command: int = struct.unpack("H", message[(len(serial_number) + 9) : (len(serial_number) + 11)])[0]
                subcommand: int = struct.unpack("H", message[(len(serial_number) + 11) : (len(serial_number) + 13)])[0]
                if command == 0x0401 or command == 0x041C:
                    if message[(len(serial_number) + 13)] == 0x00:
                        answer = extension_read_request(
                            command,
                            subcommand,
                            serial_number,
                            message[(len(serial_number) + 13) :],
                        )
                    else:
                        answer = read_request(
                            command,
                            subcommand,
                            serial_number,
                            message[(len(serial_number) + 13) :],
                        )
                    conn.sendto(answer, addr)
                elif command == 0x1401 or command == 0x141B:
                    if message[(len(serial_number) + 13)] == 0x00:
                        answer = extension_write_request(
                            command,
                            subcommand,
                            serial_number,
                            message[(len(serial_number) + 13) :],
                        )
                    else:
                        answer = write_request(
                            command,
                            subcommand,
                            serial_number,
                            message[(len(serial_number) + 13) :],
                        )
                    conn.sendto(answer, addr)
                elif command == 0x0619:
                    # loopback message
                    conn.sendto(loopback_response(serial_number, list(message)[17:]), addr)
                else:
                    raise SLMPError(serial_number, Endcode.WrongCommand)
        except SLMPError as error:
            answer = error_response(error.serial_number, error.endcode)
            try:
                conn.sendto(answer, addr)
            except BrokenPipeError:
                print(traceback.format_exc())
        except BrokenPipeError:
            print(traceback.format_exc())
        except ConnectionError:
            pass


if __name__ == "__main__":
    main()
