# Asynchronous Event Log Watcher

Most Windows event monitoring scripts rely on an infinite loop wrapping `Get-WinEvent` or `Get-WmiObject`. This is inefficient, constantly burning CPU cycles and thrashing the disk just to check if something changed.

This project exists to explore how to hook directly into the lower-level OS eventing subsystems so a script can sit entirely idle (0% CPU) until the kernel explicitly fires an interrupt to wake it up.

## Mechanisms Explored

1. **System Events (.NET Eventing Reader)**
   Instead of polling the event log, this uses `System.Diagnostics.Eventing.Reader.EventLogWatcher` to register an asynchronous callback. The script is instantly notified when specific interrupts fire, such as `Microsoft-Windows-Kernel-Power` state changes or Critical errors.

2. **Hardware Peripherals (WMI Device Events)**
   Instead of constantly querying the `Win32_PnPEntity` tree to see if a device was added, we subscribe to `Win32_DeviceChangeEvent`. The WMI provider fires an interrupt the millisecond a peripheral is mounted or unmounted from the physical bus.

3. **Process Instantiation (WMI Instance Creation)**
   By hooking `__InstanceCreationEvent` for `Win32_Process`, we intercept the exact moment the Windows kernel allocates a new process in memory, capturing its PID and Name instantly.

## Background Persistence

Rather than compiling a custom C++ executable to run as a Windows Service, this project includes a persistence script (`register_autostart.ps1`) that hooks the script directly into the Windows Task Scheduler. 

It registers an invisible task that executes at system boot under `NT AUTHORITY\SYSTEM`. This guarantees maximum privileges to hook ETW/WMI interrupts and ensures the watcher loop runs indefinitely, completely bypassing the user session boundary.

## Usage

**Interactive Mode (Testing):**
Requires an elevated administrative token to bind to kernel and security subsystems.
```powershell
.\watcher.ps1
```

**Background Service (Persistent):**
Registers the invisible SYSTEM task to run automatically on boot. Requires Administrator privileges.
```powershell
.\register_autostart.ps1
```
