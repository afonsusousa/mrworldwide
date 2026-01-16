import hid
import time
import sys
import pyautogui

VENDOR_ID  = 0x0C45
PRODUCT_ID = 0xFDFD
CMD_INIT = [0x02] + [0x00] * 30 + [0x02]
CMD_PING = [0x20, 0x01] + [0x00] * 29 + [0x21]

class DualInterfaceMonitor:
    def __init__(self, vid, pid):
        self.vid = vid
        self.pid = pid
        self.dev_ping = None   # Interface 0xFF60 (Commands)
        self.dev_listen = None # Interface 0xFFFF (Events)
        self.last_link_state = "UNKNOWN"
        self.running = True

    def find_interfaces(self):
        path_ping = None
        path_listen = None
        
        try:
            devices = hid.enumerate(self.vid, self.pid)
        except: return None, None

        for dev in devices:
            page = dev.get('usage_page', 0)
            
            if page == 0xFF60:
                path_ping = dev['path']
            elif page == 0xFFFF:
                path_listen = dev['path']

        return path_ping, path_listen

    def process_packet(self, data, source=None):

        if not data or len(data) < 4: return None

        if data[0] == 0x05 and data[1] == 0xA6:
            byte = data[3]
            if byte == 0x01 and self.last_link_state != "CONNECTED":
                print(f">>> [EVENT] KEYBOARD CONNECTED (Link Up)")
                self.last_link_state = "CONNECTED"
                pyautogui.hotkey('win', 'space')
            elif byte == 0x02 and self.last_link_state != "DISCONNECTED":
                print(f">>> [EVENT] KEYBOARD DISCONNECTED (Link Down)")
                self.last_link_state = "DISCONNECTED"
                pyautogui.hotkey('win', 'space')
            return "STATUS"
        
        elif data[0] == 0x20 and data[1] == 0x01:
            return "PONG"

        return None

    def check_initial_connection(self):
        """
        PING PONG
        """
        try:
            self.dev_ping.write([0x00] + CMD_INIT)
            time.sleep(0.05)
        except: pass

        pong_seen = False
        
        for i in range(10):
            try:
                self.dev_ping.write([0x00] + CMD_PING)
                
                data_ping = self.dev_ping.read(64, timeout_ms=50)
                if data_ping:
                    if self.process_packet(data_ping, "PING_INTERFACE") == "PONG":
                        pong_seen = True
                        break

                data_listen = self.dev_listen.read(64, timeout_ms=50)
                if data_listen:
                    self.process_packet(data_listen, "LISTEN_INTERFACE")

            except Exception as e:
                print(f"Error in initial connection check: {e}")
                pass

        if pong_seen and self.last_link_state == "UNKNOWN":
            print(f"   [INIT] Keyboard is CONNECTED")
            self.last_link_state = "CONNECTED"
        elif not pong_seen and self.last_link_state == "UNKNOWN":
            print(f"   [INIT] Keyboard is DISCONNECTED")
            self.last_link_state = "DISCONNECTED"

    def run(self):
        print(f"Starting Dual-Interface Monitor (VID={hex(self.vid)})...")
        
        while self.running:
            if not self.dev_ping or not self.dev_listen:
                path_ping, path_listen = self.find_interfaces()
                
                if path_ping and path_listen:
                    try:
                        self.dev_ping = hid.device()
                        self.dev_ping.open_path(path_ping)
                        self.dev_ping.set_nonblocking(0)
                        
                        self.dev_listen = hid.device()
                        self.dev_listen.open_path(path_listen)
                        self.dev_listen.set_nonblocking(0)

                        print(f">> Both Interfaces Opened Successfully.")
                        self.check_initial_connection()
                        print(">> Entering Passive Monitor Mode (Watching 0xFFFF)...")
                        
                    except Exception as e:
                        print(f">> Error opening interfaces: {e}")
                        self.close_all()
                else:
                    time.sleep(2)
                    continue

            # MONITOR PHASE
            try:
                data = self.dev_listen.read(64, timeout_ms=1000)
                if data:
                    self.process_packet(data, "LISTEN_INTERFACE")
                
                # Optional: We can also drain the Ping interface to keep the buffer clean
                # but we don't expect data there unless we write to it.
                
            except IOError:
                print(">> Dongle removed. Re-scanning...")
                self.close_all()
                self.last_link_state = "UNKNOWN"
            except Exception as e:
                print(f"Error in monitor loop: {e}")
                pass

    def close_all(self):
        if self.dev_ping:
            try: self.dev_ping.close()
            except: pass
        if self.dev_listen:
            try: self.dev_listen.close()
            except: pass
        self.dev_ping = None 
        self.dev_listen = None

    def stop(self):
        self.running = False
        self.close_all()

if __name__ == "__main__":
    monitor = DualInterfaceMonitor(VENDOR_ID, PRODUCT_ID)
    try:
        monitor.run()
    except KeyboardInterrupt:
        print("\nStopping...")
        monitor.stop()
        sys.exit(0)