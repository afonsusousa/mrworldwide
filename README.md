# MrWorldwide Keyboard Monitor

Got en-US Ajazz AK820 keyboard to use with a pt-PT laptop. 
This is a small reverse engineer to trigger Win+Space (switch layout shortcut) when the DONGLE detects Keyboard connection events. 
Use at your own risk.

## Dependencies

- `hidapi` - HID device communication
- `pyautogui` - hotkey triggering

## Installation

```bash
pip install hidapi pyautogui
```

## Usage

```bash
python keyboard.py
```
