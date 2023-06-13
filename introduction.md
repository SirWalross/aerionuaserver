# Introduction

Aerion is a [OPC-UA Server](https://opcfoundation.org/) for accessing data from MELFA Robots and MELSEC PLCs. It can be run on a Windows or Linux PC and is open-source.

<div class="tip custom-block" style="padding-top: 8px">

Just want to try it out? Skip to the [Installation](./installation).

</div>

## Features

- Support for OPC-UA Encryption with the `Basic256Sha256` and `Aes128Sha256RsaOaep` encryption algorithms currently supported
- Enforcement of client certificate verification supported

## MELFA Robots

Aerion supports F-Series and FR-Series MELFA Robots and mostly implements the [OPC-UA Robotics Companion Specification](https://reference.opcfoundation.org/Robotics/v100/docs). The Companion Specification defines a common layout for motion device systems like robots. The specified data can then be used from a higher level manufacturing system regardless of the model or manufacturer of a robot. 

Besides the data that is specified in the Companion Specification robot variables like the current position or more I/O can be added to the OPC-UA server with `user nodes`.

## MELSEC PLCs

Aerion currently only supports the MELSEC iQ-R PLC but other PLC support is planned. To add global labels or devices to the OPC-UA server `user nodes` are used.