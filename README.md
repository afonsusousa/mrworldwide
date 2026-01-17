# MrWorldwide Keyboard Monitor

Got en-US Ajazz AK820 keyboard to use with a pt-PT laptop. 
This is a small reverse engineer to switch between the layouts when the *DONGLE* signals Keyboard connection events. 
Use at your own risk.

## Message Structure

### Status Packet
Dongle sends status updates on the Status Interface (Page `0xFFFF`).
- **Header**: `0x05 0xA6`
- **State Byte** (Index 3):
  - `0x01`: Keyboard Connected (Link Up)
  - `0x02`: Keyboard Disconnected (Link Down)

### Ping/Pong Packet
To check connection status:
- **Ping**: Sent to Command Interface (Page `0xFF60`).
  - `0x00 0x20 0x01 ...`
- **Pong**: Received from Command Interface.
  - `0x20 0x01 ...`
