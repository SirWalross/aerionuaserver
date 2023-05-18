# Format for plc user nodes

## Name
- Name of the node
- Has to be unique for all nodes that share the same parent

## Parent
- Parent object(s) of the node
- Nested parents are split with a `/`
- Parents that are not yet present are automatically created

## Variable Type
- Type of user node
- Global Label or Device type

## Label name
- Name of the global label
- Only if user node is of type global label

## Device
- The device type
- Only if user node is of type device type

## Head no
- The number of the first device
- Only if user node is of type device type

## Datatype
- The datatype of the user node

## Writeable
- Whether the value is writeable
- At the moment only supported for non-array nodes
- Strings are only writeable for global label

## Length
- Length of the string
- Only if variable type is string and user node if type device type

## Count
- Number of elements in array
- If >1 then node is automatically set as an array node
- `[<index>]` is automatically added after the label name if set as an array