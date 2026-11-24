# ControlUnit Protocol over Moxa

This document defines the binary protocol between the PLC and the STM32 ControlUnit. The Moxa device transports the bytes between them.

## Message format

Each request and response is a 16-bit word sent as two binary bytes, most significant byte first. For example, `0xA351` is sent as `A3 51`.

The PLC sends one request at a time and waits for the response before sending the next one. Fields shown as `0` are reserved and must be zero.

## Commands

| Word     | Command            | Description                                                                             |
| -------- | ------------------ | --------------------------------------------------------------------------------------- |
| `0x0000` | `INIT`             | Starts communication. Leaves rail power and stored faults as they are.                  |
| `0x0100` | `RUN_KEEPALIVE`    | Refreshes the operating watchdog and returns ACK.                                        |
| `0x1I00` | `GET_STATUS`       | Reads the value of status ID `I`.                                                       |
| `0x2IVV` | `SET_STATUS`       | Writes value `VV` to writable status ID `I`.                                            |
| `0xACSL` | `SEND_CAR_COMMAND` | Sets the speed of car `C` and optionally requests a lane change.                        |
| `0xF000` | `RESET_ERROR`      | Acknowledges the stored fault and leaves the rail output OFF.                           |
| `0xFFFF` | `ENTER_ERROR_MODE` | Switches the rail output OFF and enters error mode. Preserves any fault already stored. |

`I`, `C`, `S`, and `L` are each 4 bits wide. `VV` is 8 bits wide.

## Status IDs

| ID    | Status       | Access       | Values                                                                                                       |
| ----- | ------------ | ------------ | ------------------------------------------------------------------------------------------------------------ |
| `0x0` | Rail power   | Read / write | Read `0x00` = OFF, `0x01` = ON, `0x02` = error mode. Write `0x00` to switch OFF or `0x01` to request enable. |
| `0x1` | Stored fault | Read only    | `0x00` = no fault, `0x01` = rail short circuit, `0x02` = shutdown requested by the PLC or rail signal timing fault, `0x03` = operating heartbeat expired.                      |

An enable request is rejected while a fault is stored. To restart after a fault, the PLC sends `RESET_ERROR`, waits for `ACK`, then sends `SET_STATUS` with rail power set to `0x01`.

## Car command fields

Car commands are accepted while rail power is ON. The STM32 keeps the last accepted speed for each car in RAM and includes it in every CAR packet for that address. A new car command changes only the addressed car. The PLC sends a car command when the setting changes and keeps sending the operating heartbeat. Enabling the rail starts all cars at speed zero with their lane-change requests cleared.

| Field | Description                                                                      |
| ----- | -------------------------------------------------------------------------------- |
| `C`   | OpenSlot address `0x0` through `0x5` for Carrera cars 1 through 6. Other addresses are invalid.                     |
| `S`   | Speed step from `0x0` to `0xF`. `0x0` means stop.                                |
| `L`   | `0x0` = no lane change, `0x1` = request a lane change. Other values are invalid. |

A lane change is requested when `L` changes from `0` to `1` for that car. The initial value is `0`. Repeated commands with `L = 1` do not request another lane change. Send `L = 0` before requesting the next one. Each request presses the lane-change bit in one CAR packet and releases it in the next. Each car can hold one pending request. A new rising edge while that request is pending returns `INVALID_STATE` and can be retried.

Example `0xA351` sets Carrera car 4 (address `3`) to speed step `5` and requests a lane change if its previous `L` was `0`.

## Responses

| Word     | Response           | Description                                                          |
| -------- | ------------------ | -------------------------------------------------------------------- |
| `0x3000` | `ACK`              | The command was accepted.                                            |
| `0x3001` | `UNKNOWN_COMMAND`  | The command is unsupported.                                          |
| `0x3002` | `INVALID_ARGUMENT` | A field, status ID, or value is invalid, or the status is read only. |
| `0x3003` | `INVALID_STATE`    | The command is unavailable in the current state.                     |
| `0x4IVV` | `STATUS_VALUE`     | Response to `GET_STATUS` with status ID `I` and value `VV`.          |

Example `GET_STATUS` with word `0x1000` returns `0x4002` when the rail is in error mode.

## UART example

Use USART2 at 115200 baud, 8 data bits, no parity, 1 stop bit, and no flow control. Send two binary bytes per request and wait for the two-byte reply.

| Send (hex) | Reply (hex) | Action |
| --- | --- | --- |
| `00 00` | `30 00` | Check communication with INIT. |
| `01 00` | `30 00` | Start or refresh the operating heartbeat. |
| `20 01` | `30 00` | Enable the rail. |
| `A3 50` | `30 00` | Set Carrera car 4 to speed step 5. |
| `A3 00` | `30 00` | Set Carrera car 4 to speed step 0. |
| `20 00` | `30 00` | Switch off the rail. |

Send `01 00` every 250 ms during operation. `30 00` acknowledges receipt by the STM32.

## Timeout and synchronization

The PLC sends `RUN_KEEPALIVE` every 250 ms during operation, provided it has received valid responses. The ControlUnit accepts a rail-enable request only after receiving a heartbeat. Its watchdog starts on the first heartbeat. If no further heartbeat arrives within 1000 ms, the ControlUnit locks the rail output and stores fault `0x03` if no fault is already stored.

If the PLC stops receiving valid responses, it switches its local outputs off and stops sending heartbeats and control commands. It polls `GET_STATUS` (`0x1000`) to detect recovery. Status requests do not refresh the ControlUnit watchdog. After detection, the PLC can resume heartbeats. To restart the rail, it sends `RESET_ERROR`, waits for `ACK`, then sends a separate enable request.

The receiver collects two bytes before decoding a word. This format assumes both sides start on a word boundary. If a byte is lost or a response times out, reset the communication state on both ends before sending `INIT` again. Two-byte words alone cannot recover a lost word boundary.
