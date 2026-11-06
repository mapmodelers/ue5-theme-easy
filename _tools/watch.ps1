# Easy Theme watcher: runs ONLY the actions listed below when _run\request.txt appears (dev loop).
# Results: _run\done.txt (action, exit code, time), _run\watch.log; heartbeat in _run\alive.txt.
$root = Split-Path -Parent $PSScriptRoot
$run = Join-Path $root '_run'
New-Item -ItemType Directory -Force $run | Out-Null
$req = Join-Path $run 'request.txt'

$actions = @{
    'go'    = { & cmd.exe /c "`"$root\go.cmd`"" }
    'close' = { & powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$root\_tools\close_editor.ps1" }
}

$host.UI.RawUI.WindowTitle = 'Easy Theme watcher'
Write-Host 'Easy Theme watcher - leave this window open. Close it to stop.'
Write-Host ('Allowed actions: ' + ($actions.Keys -join ', '))
$beat = 0
while ($true) {
    if (Test-Path $req) {
        Start-Sleep -Milliseconds 300
        $a = ''
        try { $a = ((Get-Content $req -TotalCount 1) -as [string]).Trim().ToLower() } catch {}
        Remove-Item $req -Force -ErrorAction SilentlyContinue
        $stamp = Get-Date -Format s
        if ($actions.ContainsKey($a)) {
            Write-Host "[$stamp] running: $a"
            & $actions[$a] 2>&1 | Tee-Object -FilePath (Join-Path $run 'watch.log')
            $code = $LASTEXITCODE
        } else {
            Write-Host "[$stamp] refused unknown action: $a"
            $code = 'refused'
        }
        Set-Content -Path (Join-Path $run 'done.txt') -Value "$a $code $(Get-Date -Format s)"
        Write-Host "[$(Get-Date -Format s)] finished: $a ($code)"
    }
    if ($beat % 5 -eq 0) { Set-Content -Path (Join-Path $run 'alive.txt') -Value (Get-Date -Format s) }
    $beat++
    Start-Sleep -Seconds 2
}
