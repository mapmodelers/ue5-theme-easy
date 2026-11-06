# Easy Theme updater. Started by the plugin when Unreal closes, after you accepted an update.
# 1. waits for that editor to exit  2. checks the download's SHA-256  3. backs up the current
# plugin  4. replaces it (asks for admin rights if the plugin is in Program Files)
# 5. restores the backup if anything fails. Log: update.log next to the download.
param(
    [string]$Zip, [string]$ShaFile, [string]$PluginDir, [int]$EditorPid = 0,
    [string]$Version, [switch]$Elevated, [switch]$Quiet
)
$ErrorActionPreference = 'Stop'
$UpdateDir = Split-Path -Parent $Zip
$Log = Join-Path $UpdateDir 'update.log'
function Log([string]$m) { Add-Content -Path $Log -Value ('[{0}] {1}' -f (Get-Date -Format s), $m) }
function Say([string]$m, [string]$icon = 'Information') {
    Log "message: $m"
    if ($Quiet) { return }
    Add-Type -AssemblyName System.Windows.Forms
    [void][System.Windows.Forms.MessageBox]::Show($m, 'Easy Theme update', 'OK', $icon)
}
function Copy-Tree([string]$From, [string]$To) {
    robocopy $From $To /MIR /XD Intermediate /R:24 /W:5 /NFL /NDL /NJH /NJS /NP | Out-Null
    return ($LASTEXITCODE -lt 8)
}

try {
    Log "start: version $Version, plugin $PluginDir, pid $EditorPid, elevated $Elevated"
    if ($EditorPid -gt 0 -and -not $Elevated) {
        try { Wait-Process -Id $EditorPid -Timeout 900 -ErrorAction SilentlyContinue } catch {}
    }

    $expected = ((Get-Content -Raw -Path $ShaFile).Trim() -split '\s+')[0].ToLowerInvariant()
    $actual = (Get-FileHash -Algorithm SHA256 -Path $Zip).Hash.ToLowerInvariant()
    if ($expected -ne $actual) {
        Log "hash mismatch: expected $expected got $actual"
        Say 'Update refused: the download is damaged or was altered. Nothing was changed.' 'Error'
        exit 1
    }
    Log 'hash ok'

    # Program Files needs admin rights: run again elevated (Windows asks once).
    $probe = Join-Path $PluginDir '.easytheme_write_test'
    try { [IO.File]::WriteAllText($probe, 'x'); Remove-Item -Force $probe }
    catch {
        if ($Elevated) { throw 'no write access to the plugin folder' }
        Log 'elevating'
        $a = "-NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File `"$PSCommandPath`" -Zip `"$Zip`" -ShaFile `"$ShaFile`" -PluginDir `"$PluginDir`" -Version `"$Version`" -Elevated"
        if ($Quiet) { $a += ' -Quiet' }
        Start-Process -FilePath 'powershell.exe' -Verb RunAs -WindowStyle Hidden -ArgumentList $a
        exit 0
    }

    $work = Join-Path $env:TEMP ('EasyThemeUpdate_' + [guid]::NewGuid().ToString('N'))
    Expand-Archive -Path $Zip -DestinationPath $work -Force
    $src = Join-Path $work 'EasyTheme'
    if (-not (Test-Path (Join-Path $src 'EasyTheme.uplugin'))) { throw 'the download has no EasyTheme.uplugin' }

    $backup = Join-Path $UpdateDir 'Backup'
    if (Test-Path $backup) { Remove-Item -Recurse -Force $backup }
    if (Test-Path (Join-Path $PluginDir 'EasyTheme.uplugin')) {
        if (-not (Copy-Tree $PluginDir $backup)) { throw 'could not back up the current version' }
        Log "backup: $backup"
    }

    if (-not (Copy-Tree $src $PluginDir)) {
        Log 'copy failed (files in use?), restoring backup'
        if (Test-Path $backup) { [void](Copy-Tree $backup $PluginDir) }
        Say 'Easy Theme could not be updated: another Unreal editor is still using it. Your previous version was kept. Close every Unreal editor, then use Check for Updates again.' 'Warning'
        exit 1
    }
    Remove-Item -Recurse -Force $work -ErrorAction SilentlyContinue
    Remove-Item -Force $Zip, $ShaFile -ErrorAction SilentlyContinue
    Log "updated to $Version"
    Say "Easy Theme was updated to $Version. It is active the next time you open Unreal."
    exit 0
}
catch {
    Log "error: $_"
    Say "Easy Theme update failed: $_`nYour current version was kept." 'Error'
    exit 1
}
