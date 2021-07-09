# Redirection of AFD Sockets via Process Monitoring / Injection
import os
import time
from ProcessMonitor import ProcessMonitor
from ContextHijack import inject

# Get Our Socket Library Paths
base_path = os.path.dirname(os.path.abspath(__file__))
AFDPROXY_32_PATH = os.path.join(base_path, "deps", "afdproxy32.dll")
AFDPROXY_64_PATH = os.path.join(base_path, "deps", "afdproxy64.dll")


# Write our Injection Callback
def inject_afdproxy(proc_info):
    inject([AFDPROXY_32_PATH, AFDPROXY_64_PATH, proc_info['pid']])


class SocketRedirector(object):
    def __init__(self, targets, child_aware=False, full_paths=False):
        self.ctx = ProcessMonitor(targets, inject_afdproxy, child_aware, full_paths)

    def __del__(self):
        del self.ctx


if __name__ == "__main__":
    sr = SocketRedirector(["notepad.exe"], True, False)
    time.sleep(20)
    del sr
