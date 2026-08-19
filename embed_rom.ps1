$in = $args[0]
$name = $args[1]
$bytes = [System.IO.File]::ReadAllBytes((Resolve-Path $in))

$out = "#ifndef ROM_${name}_H`n#define ROM_${name}_H`n`n#include <stdint.h>`n`n"
$out += "static const uint8_t rom_${name}[] = {`n"

for ($i = 0; $i -lt $bytes.Length; $i += 16) {
    $line = "   "
    for ($j = $i; $j -lt [Math]::Min($i+16, $bytes.Length); $j++) {
        $line += "0x{0:X2}, " -f $bytes[$j]
    }
    $out += $line + "`n"
}

$out += "};`n"
$out += "static const uint32_t rom_${name}_size = $($bytes.Length);`n"
$out += "`n#endif`n"

Set-Content -Path $args[2] -Value $out -Encoding UTF8
Write-Output "$($bytes.Length) bytes -> $($args[2])"