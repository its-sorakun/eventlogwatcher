# Registering a SYSTEM-level scheduled task requires an elevated security token.
$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Start-Process powershell -ArgumentList "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`"" -Verb RunAs
    exit
}

$TaskName = "EventLogWatcher_BackgroundService"
$WatcherScript = Join-Path $PSScriptRoot "watcher.ps1"

# Suppressing the console window allocation and running non-interactively prevents the host from flashing a black box when the task initializes.
$Action = New-ScheduledTaskAction -Execute "powershell.exe" -Argument "-WindowStyle Hidden -NonInteractive -ExecutionPolicy Bypass -File `"$WatcherScript`""

# Hooking the task to system boot ensures the watcher initializes before the login screen even appears.
$Trigger = New-ScheduledTaskTrigger -AtStartup

# Running as NT AUTHORITY\SYSTEM grants the script maximum privileges to hook kernel ETW events and WMI, and allows it to run regardless of whether any human user is actively logged into the desktop session.
$Principal = New-ScheduledTaskPrincipal -UserId "NT AUTHORITY\SYSTEM" -LogonType ServiceAccount -RunLevel Highest

# Default Task Scheduler settings aggressively kill tasks that run longer than 72 hours or when running on battery power. Overriding these settings ensures the watcher loop runs indefinitely.
$Settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -ExecutionTimeLimit 0

Write-Host "Registering background service task: $TaskName" -ForegroundColor Cyan
Register-ScheduledTask -TaskName $TaskName -Action $Action -Trigger $Trigger -Principal $Principal -Settings $Settings -Force | Out-Null

Write-Host "Task registered successfully. Starting the task now..." -ForegroundColor Green
Start-ScheduledTask -TaskName $TaskName

Write-Host "Watcher is now running silently in the background as NT AUTHORITY\SYSTEM." -ForegroundColor Yellow
