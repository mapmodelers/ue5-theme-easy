# BuildPlugin drops "EnabledByDefault" from the packaged descriptor; put it back so an
# engine-wide install (Engine/Plugins/Marketplace) is active in every project.
param([string]$Path)
$s = [IO.File]::ReadAllText($Path)
if ($s -notmatch '"EnabledByDefault"') {
    $s = [regex]::Replace($s, '("Installed":\s*true,)', ('$1' + "`n`t" + '"EnabledByDefault": true,'))
    [IO.File]::WriteAllText($Path, $s)
}
