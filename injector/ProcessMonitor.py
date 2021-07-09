# Process Monitor Daemon Class
# Given a Set of Targets and a Callback, Monitor Processes for Create and Perform Callback
from threading import Thread
from ProcessMonitor.pywinapi.psapi import enum_processes
from ProcessMonitor.ProcessInfo import ProcessInfo


class ProcessMonitor(object):
    def __init__(self, targets, callback_fcn, child_aware=False, match_full_paths=False):
        self.trigger_children = child_aware
        self.is_running = False
        # Sanitize Targets
        self.targets = []
        for t in targets:
            self.targets.append(t.lower())
        # If we should be checking the target against full paths or just names.
        self.match_full_paths = match_full_paths
        self.callback_fcn = callback_fcn
        self.triggered_pids = set()
        self.processed_pids = set()
        self.scan_thread = Thread(target=self.monitor, )
        self.scan_thread.daemon = True
        self.scan_thread.start()

    def is_target(self, pi):
        # We care about this process if it's a child of a triggered process.
        # And we previously specified that we want to recursively trigger on children.
        if self.trigger_children:
            if pi.parent_pid in self.triggered_pids:
                return True

        # We then check either the path or name to determine if this is a target we care about.
        if self.match_full_paths:
            if pi.path in self.targets:
                return True
        else:
            if pi.name in self.targets:
                return True
        return False

    def monitor(self):
        print("Starting Up Process Monitor")
        self.is_running = True
        while self.is_running is True:
            res, current_pid_list = enum_processes()
            if not res:
                continue

            # We only care about new processes since the last iteration.
            new_pids = set(current_pid_list).difference(self.processed_pids)
            for pid in new_pids:
                pi = ProcessInfo(pid)
                if not pi.valid:
                    continue

                # If this is a desired target, add it and link the callback.
                if self.is_target(pi):
                    self.triggered_pids.add(pid)
                    self.callback_fcn(pi)

            # Overwrite the Current PID Set
            self.processed_pids = set(current_pid_list)

    def __del__(self):
        print("Shutting Down Process Monitor")
        self.is_running = False
