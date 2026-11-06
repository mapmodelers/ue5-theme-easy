# Closes only Unreal editors opened on this repo's test project.
$p = Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe'" | Where-Object { $_.CommandLine -like '*ue5-theme-easy*' }
foreach ($x in $p) {
    Stop-Process -Id $x.ProcessId -Force
    Write-Output "closed editor $($x.ProcessId)"
}
if ($p) { Start-Sleep -Seconds 2 }
