# Injection Module via SetContext
import struct

from pycore.pywinapi.kernel32.processthreadsapi import open_process_rw
from pycore.pywinapi.kernel32.handleapi import close_handle
from pycore.pywinapi.kernel32.processthreadsapi import open_thread_ctx_rw
from pycore.pywinapi.kernel32.processthreadsapi import get_thread_context
from pycore.pywinapi.kernel32.processthreadsapi import set_thread_context
from pycore.pywinapi.kernel32.processthreadsapi import is_thread_suspended
from pycore.pywinapi.kernel32.processthreadsapi import suspend_thread
from pycore.pywinapi.kernel32.processthreadsapi import resume_thread
from pycore.pywinapi.kernel32.processthreadsapi import get_thread_cip
from pycore.pywinapi.kernel32.processthreadsapi import set_thread_cip
from pycore.pywinapi.kernel32.tlhelp32 import get_main_thread_id
from pycore.pywinapi.kernel32.processthreadsapi import create_process_suspended
from pycore.pywinapi.kernel32.wow64apiset import is_wow64_process
from pycore.pywinapi.kernel32.libloaderapi import get_library_address
from pycore.pywinapi.kernel32.memoryapi import allocate_rw_memory
from pycore.pywinapi.kernel32.memoryapi import allocate_rwx_memory
from pycore.pywinapi.kernel32.memoryapi import write_memory
from pycore.pywinapi.kernel32.memoryapi import set_memory_protect_ro
from pycore.pywinapi.kernel32.memoryapi import set_memory_protect_x
from ContextHijack.LLA32Resolver import LLA32Resolver


def c_str(instr):
    return instr.encode('ascii') + b"\x00"


# Gets a WOW64 Address from Kernel32 for a given proc.
def get_lla_address_external(proc_name):
    r = LLA32Resolver()

    try:
        return r.address
    except Exception as e:
        return 0


# Kernel32 by default lives in the same address space for every process per session.
CACHED_LOADLIBRARYA_WOW64 = get_lla_address_external("LoadLibraryA")


def get_lla_address(is_wow64):
    global CACHED_LOADLIBRARYA_WOW64
    if is_wow64:
        return True, CACHED_LOADLIBRARYA_WOW64
    else:
        return get_library_address("kernel32.dll", "LoadLibraryA")



def Generate_ShellCode_x86(dll_path_addr, lla_addr, ret_addr):
    # pushad
    code = b"\x60"
    # push [dll_path_addr]
    code += b"\x68" + struct.pack("<I", dll_path_addr)
    # mov eax, [loadlibrary_addr]
    code += b"\xB8" + struct.pack("<I", lla_addr)
    # call eax
    code += b"\xFF\xD0"
    # popad
    code += b"\x61"
    # push [ret_addr]
    code += b"\x68" + struct.pack("<I", ret_addr)
    # ret
    code += b"\xC3"

    return code


def Generate_ShellCode_x64(dll_path_addr, lla_addr, ret_addr):
    # sub rsp, 0x28
    code = b"\x48\x83\xEC\x28"
    # mov [rsp + 18], rax
    code += b"\x48\x89\x44\x24\x18"
    # mov [rsp + 0x10], rcx
    code += b"\x48\x89\x4C\x24\x10"
    # mov rcx, [dll_path_addr]
    code += b"\x48\xB9" + struct.pack("<Q", dll_path_addr)
    # mov rax, [loadlibrary_addr]
    code += b"\x48\xB8" + struct.pack("<Q", lla_addr)
    # call rax
    code += b"\xFF\xD0"
    # mov rcx, [rsp + 0x10]
    code += b"\x48\x8B\x4C\x24\x10"
    # mov rax, [rsp + 0x18]
    code += b"\x48\x8B\x44\x24\x18"
    # add rsp, 0x28
    code += b"\x48\x83\xC4\x28"
    # mov r11, [ret_addr]
    code += b"\x49\xBB" + struct.pack("<Q", ret_addr)
    # jmp r11
    code += b"\x41\xFF\xE3"

    return code


def usage():
    print("Usage: target 32_dll_path 64_dll_path")


def inject(args):
    try:
        dll_32 = args[0]
        dll_64 = args[1]
        target = args[2]
    except:
        print("Load Error - Missing Arguments")
        usage()
        return False

    # First, Determine if We are Targeting a Process to be Created or a Running Process

    try:
        target_pid = int(target)
        target_path = ""
        is_running = True
    except ValueError:
        target_path = target
        target_pid = -1
        is_running = False

    # Dependent upon the target, our initial state needs to be uniform.
    # We need a handle to the process, a handle to the main thread
    # and we need to suspend the main thread.

    if is_running:
        res, h_process = open_process_rw(target_pid)
        if not res:
            print("open_process failed!")
            return False

        # Get the Main Thread ID from our pid
        res, tid = get_main_thread_id(target_pid)
        if not res:
            print("get_main_thread_id failed!")
            close_handle(h_process)
            return False

        # Get a Handle to Our Main Thread
        res, h_thread = open_thread_ctx_rw(tid)
        if not res:
            print("open_thread_ctx_rw failed!")
            close_handle(h_process)
            return False


    else:
        # Get the handle of our created process and thread
        res, si, pi = create_process_suspended(target)
        if not res:
            print("create_process_suspended failed!")
            return False
        h_process = pi.hProcess
        h_thread = pi.hThread

    # Get our Process Architecture
    res, is_wow64 = is_wow64_process(h_process)
    if not res:
        print("is_wow64_process failed!")
        close_handle(h_process)
        return False

    # Determine if our thread is suspended
    res, is_suspended = is_thread_suspended(h_thread, is_wow64)
    if not res:
        print("is_thread_suspended failed!")
        close_handle(h_process)
        close_handle(h_thread)
        return False

    # Suspend it if it isn't
    if not is_suspended:
        res = suspend_thread(h_thread, is_wow64)
        if not res:
            print("suspend_thread failed!")
            close_handle(h_process)
            close_handle(h_thread)
            return False

    # Get Thread Context
    res, tctx = get_thread_context(h_thread, is_wow64)
    if not res:
        print("get_thread_context failed!")
        close_handle(h_process)
        close_handle(h_thread)
        return False

    # Get Current Instruction Pointer
    ret_addr = get_thread_cip(tctx, is_wow64)

    # Get LoadLibraryA Address
    res, lla_addr = get_lla_address(is_wow64)
    if not res:
        print("get_lla_address failed!")
        close_handle(h_process)
        close_handle(h_thread)
        return False

    # Allocate and Write DLL Path Address
    if is_wow64:
        bdll_path = c_str(dll_32)
    else:
        bdll_path = c_str(dll_64)

    res, dll_path_addr = allocate_rw_memory(h_process, len(bdll_path))
    if not res:
        print("allocate_rw_memory failed!")
        close_handle(h_process)
        close_handle(h_thread)
        return False

    if not write_memory(h_process, dll_path_addr, bdll_path)[0]:
        print("write_memory failed!")
        close_handle(h_process)
        close_handle(h_thread)
        return False

    if not set_memory_protect_ro(h_process, dll_path_addr, len(bdll_path))[0]:
        print("set_memory_protect_ro failed!")
        close_handle(h_process)
        close_handle(h_thread)
        return False

    # Create ShellCode
    if is_wow64:
        shellcode_payload = Generate_ShellCode_x86(dll_path_addr, lla_addr, ret_addr)
    else:
        shellcode_payload = Generate_ShellCode_x64(dll_path_addr, lla_addr, ret_addr)

    # Allocate and Write ShellCode
    res, shellcode_addr = allocate_rwx_memory(h_process, len(shellcode_payload))
    if not res:
        print("allocate_rwx_memory failed!")
        close_handle(h_process)
        close_handle(h_thread)
        return False

    res, bytes_written = write_memory(h_process, shellcode_addr, shellcode_payload)
    if not res or bytes_written != len(shellcode_payload):
        print("write_memory failed!")
        close_handle(h_process)
        close_handle(h_thread)
        return False

    if not set_memory_protect_x(h_process, shellcode_addr, len(shellcode_payload))[0]:
        print("set_memory_protect_x failed!")
        close_handle(h_process)
        close_handle(h_thread)
        return False

    # Set Thread Context to Shellcode Address
    tctx = set_thread_cip(tctx, shellcode_addr, is_wow64)

    if not set_thread_context(h_thread, tctx, is_wow64):
        print("set_thread_context failed!")
        close_handle(h_process)
        close_handle(h_thread)
        return False

    # Resume the Thread
    res = resume_thread(h_thread)

    # Clean up And Scoot!
    close_handle(h_process)
    close_handle(h_thread)

    return res

