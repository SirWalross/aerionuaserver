export const plc_module_io = {
    0x03ff: "Control CPU",
    0x03e0: "Multiple system CPU No.1",
    0x03e1: "Multiple system CPU No.2",
    0x03e2: "Multiple system CPU No.3",
    0x03e3: "Multiple system CPU No.4"
}

export const robot_datatype = {
    Position: "Position",
    Joint: "Joint",
    Int32: "Integer",
    Double: "Double",
    HexInt32: "Hexadecimal Integer",
    Bool: "Boolean",
    String: "String"
}

export const plc_datatype = {
    BOOL: "Bool",
    WORD: "Word",
    DWORD: "DWord",
    INT: "Int",
    DINT: "DInt",
    REAL: "Float",
    LREAL: "Double",
    // TIME: "Time"
    // STRING: "String",
    // WSTRING: "Unicode String",
    // TIMER: "Timer",
    // COUNTER: "Counter",
    // POINTER: "Pointer"
}

export const plc_device_type = {
    0xA8: "D",
    0x9C: "X",
    0x9D: "Y",
    0x90: "M",
    0xB4: "W",
    0x91: "SM",
    0xA9: "SD",
    0x92: "L"
}

export const plc_variable_type = {
    GlobalLabel: "Global Label",
    Device: "Device"
}