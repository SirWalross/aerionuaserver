# Format for robot user nodes

## Name
- Name of the node
- Has to be unique for all nodes that share the same parent

## Parent
- Parent object(s) of the node
- Nested parents are split with a `/`
- Parents that are not yet present in the OPC-UA Robotics Companion Specification are automatically created
- Examples: `Output` for new outputs or `MotionDevices/MotionDevice_1/ParameterSet` for new parameters for the first motion device

## Datatype
- The datatype of the variable
- Positions and Joints are encoded as arrays of doubles with length 10 and 8 respectively 
- For positions the format is `x, y, z, a, b, c, l1, l2, fl1, fl2`
- For joints the format is `j1, j2, j3, j4, j5, j6, j7, j8`

## ReadCommand command
- The r3 command that gets sent to the robot to read the value
- The mecha number and task slot at the beginning is optional (if not provided 1;1; is used as the default)
- Examples: `VALP_Curr` returns the current position of the robot, `OUT64` returns the value of the 16 outputs 64-79 as hexadecimal number

## ReadCommand match
- The regular expression match to extract the value from the answer received from the robot
- See [re2](https://github.com/google/re2/wiki/Syntax) for information about regular expression syntax (regex for short)
- The value from the first capture group is used as the value
- The answer that the match is matched against doesn't include the QoK
- Examples: 
	- `^P_Curr=(.*)` captures the value of the position that is after the equal sign
	- `^([^;]*);?` captures the value until the first semicolon
	- `^(?:[^;]*;){{7}}([^;]*);?` captures the value after the 7th semicolon
- Note: quantifiers in regex like `{2}` have to be replaced with `{{2}}`

## Writeable
- Whether the value is writeable
- Have to provide a WriteCommand if it is
- Writeable variables are only supported for the datatypes `Integer`, `Hexadecimal Integer` and `Double`

## WriteCommand
- The r3 command that gets sent to the robot to write the value
- The mecha number and task slot at the beginning is optional (if not provided 1;1; is used as the default)
- The to be written value is inserted in place of `{value}`
- Examples:
	- `1;9;VAL=M1={value}` assigns the value to M1
	- `OUT=64;{value:04x}` writes the value to the outputs 64-79 as a 4 digit hexadecimal number
- Format specifiers for formatting the value are supported
- For the supported syntax see [fmt format specifiers](https://fmt.dev/latest/syntax.html)