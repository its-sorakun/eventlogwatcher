# Security log read access requires elevated tokens. Restarting the process with Administrator privileges
# prevents silent failures during event subscription initialization.
$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Start-Process powershell -ArgumentList "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`"" -Verb RunAs
    exit
}

$global:LogFile = Join-Path $PSScriptRoot "events.log"

function global:Write-Alert {
    param(
        [string]$Category,
        [string]$Message,
        [ConsoleColor]$Color = "White"
    )
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $formatted = "[$timestamp] [$Category] $Message"
    
    # Console output uses direct .NET calls to ensure it prints from background runspaces
    [Console]::ForegroundColor = $Color
    [Console]::WriteLine($formatted)
    [Console]::ResetColor()
    
    # Persistent log maintains historical record across script restarts
    Add-Content -Path $global:LogFile -Value $formatted
}

Write-Alert "SYSTEM" "Event Log Watcher initialized. Subscribing to kernel interrupts..." "Cyan"

# Region: System Events (Errors, Critical, Kernel-Power)
# Polling Get-WinEvent burns CPU cycles. Using EventLogWatcher registers an asynchronous callback 
# with the OS eventing subsystem, allowing the thread to idle until an interrupt occurs.
$systemQueryStr = "*[System[(Level=1 or Level=2) or Provider[@Name='Microsoft-Windows-Kernel-Power']]]"
$systemQuery = New-Object System.Diagnostics.Eventing.Reader.EventLogQuery('System', [System.Diagnostics.Eventing.Reader.PathType]::LogName, $systemQueryStr)
$systemWatcher = New-Object System.Diagnostics.Eventing.Reader.EventLogWatcher($systemQuery)

Register-ObjectEvent -InputObject $systemWatcher -EventName 'EventRecordWritten' -SourceIdentifier 'SystemEvents' -Action {
    $record = $Event.SourceEventArgs.EventRecord
    $level = $record.LevelDisplayName
    $provider = $record.ProviderName
    $id = $record.Id
    
    $color = "Red"
    if ($provider -eq "Microsoft-Windows-Kernel-Power") {
        $color = "Yellow"
    }
    
    Write-Alert "SYSTEM_EVENT" "Level: $level | Provider: $provider | ID: $id" $color
} | Out-Null
$systemWatcher.Enabled = $true

# Region: Process Creation (System Internals)
# Since Logon events require Audit Policies to be enabled (and a password to actually 'lock' the session), 
# we will monitor Process Creation instead. WMI __InstanceCreationEvent fires asynchronously when 
# the OS kernel allocates a new Win32_Process.
$processQuery = "SELECT * FROM __InstanceCreationEvent WITHIN 1 WHERE TargetInstance ISA 'Win32_Process'"
Register-WmiEvent -Query $processQuery -SourceIdentifier 'ProcessMonitor' -Action {
    try {
        $process = $Event.SourceEventArgs.NewEvent.TargetInstance
        $name = $process.Name
        $processId = $process.ProcessId
        
        # Filtering out extreme background noise to make user-launched processes visible
        if ($name -notmatch "svchost|conhost|WmiPrvSE|taskeng|backgroundTaskHost|RuntimeBroker") {
            Write-Alert "SYSTEM_PROCESS" "Process Created | PID: $processId | Name: $name" "Yellow"
        }
    } catch {
        Write-Alert "ERROR" "Process Monitor Error: $_" "Red"
    }
} | Out-Null

# Region: USB Storage Plug/Unplug
# Win32_VolumeChangeEvent only fires for storage drives. Win32_DeviceChangeEvent fires for ALL 
# peripherals (mice, keyboards, phones, etc.) making it much more robust for general hardware detection.
Register-WmiEvent -Class Win32_DeviceChangeEvent -SourceIdentifier 'USBMonitor' -Action {
    try {
        $event = $Event.SourceEventArgs.NewEvent
        
        # EventType 2 is Arrival (Inserted), 3 is Removal (Ejected)
        if ($event.EventType -eq 2 -or $event.EventType -eq 3) {
            $action = if ($event.EventType -eq 2) { "INSERTED" } else { "REMOVED" }
            $color = if ($action -eq "INSERTED") { "Green" } else { "DarkGray" }
            
            Write-Alert "HARDWARE_DEVICE" "Action: $action | Peripheral state changed" $color
        }
    } catch {
        Write-Alert "ERROR" "WMI Action Error: $_" "Red"
    }
} | Out-Null

Write-Alert "SYSTEM" "Subscriptions active. Listening for events. Press Ctrl+C to exit." "Cyan"

# Keeping the runspace alive is required to process the asynchronous callbacks.
try {
    while ($true) {
        Start-Sleep -Seconds 1
    }
}
finally {
    # Unregistering event subscriptions ensures OS resources are released when the process terminates.
    Write-Alert "SYSTEM" "Unregistering event subscriptions..." "Cyan"
    Unregister-Event -SourceIdentifier 'SystemEvents' -ErrorAction SilentlyContinue
    Unregister-Event -SourceIdentifier 'ProcessMonitor' -ErrorAction SilentlyContinue
    Unregister-Event -SourceIdentifier 'USBMonitor' -ErrorAction SilentlyContinue
    if ($systemWatcher) { $systemWatcher.Dispose() }
}
