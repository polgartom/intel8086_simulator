<?php

const COLOR_DEFAULT = "\033[0m";
const COLOR_RED     = "\033[0;31m";
const COLOR_GREEN   = "\033[0;32m";

function pout(array $output) {
    global $argv;
    if (@$argv[1] === '-q') return '';
    return implode("\n", $output); 
}

$strict_mode = false;
foreach ($argv as $v) { if($v==='--strict') { $strict_mode = true; break; } }

exec('call .\build.bat', $out, $code);
if ($code !== 0) { 
    echo pout($out); 
    printf("\n\n%s[ERROR]: C compilation is failed!%s\n", COLOR_RED, COLOR_DEFAULT);
    exit(1); 
}

exec('call .\build\assembler.exe', $out, $code);
echo pout($out);
if ($code !== 0) { 
    exit(1); 
}

exec('nasm.exe mock\listing_0039_more_movs.asm', $out, $code);
if ($code !== 0) { 
    echo pout($out); 
    printf("\n\n%s[ERROR]: NASM compilation is failed!%s\n", COLOR_RED, COLOR_DEFAULT);
    exit(1); 
}

// exec('call .\build\main.exe .\mock\a.out --decode --hide_inst_mem_addr', $myassembler_output, $code);
exec('call .\build\main.exe .\mock\a.out --decode ' . ($strict_mode ? '--show_raw_bin' : ''), $myassembler_output, $code);
$myassembler_output = pout($myassembler_output);

// exec('call .\build\main.exe .\mock\listing_0039_more_movs --decode --hide_inst_mem_addr', $nasm_output, $code);
exec('call .\build\main.exe .\mock\listing_0039_more_movs --decode ' . ($strict_mode ? '--show_raw_bin' : ''), $nasm_output, $code);
$nasm_output = pout($nasm_output);

exec('call hexdump .\mock\a.out', $ha, $code);
exec('call hexdump .\mock\listing_0039_more_movs', $hb, $code);

foreach ($ha as $i => $x) { $ha[$i] = substr($ha[$i], strpos($ha[$i], ' ', 0)); }
foreach ($hb as $i => $x) { $hb[$i] = substr($hb[$i], strpos($hb[$i], ' ', 0)); }
$ha = explode(' ', implode("\n", $ha));
$hb = explode(' ', implode("\n", $hb));

printf("\n---------HEX---------\n\n");

$len = min(count($ha), count($hb));
for ($i = 0; $i < $len; $i++) {
    if ($ha[$i] !== $hb[$i]) {
        printf("%s%s%s|%s%s%s ", COLOR_RED, $ha[$i], COLOR_DEFAULT, COLOR_GREEN, $hb[$i], COLOR_DEFAULT);
    } else {
        printf("%s ", $ha[$i]);
    }

    if (!($i+1 % 16)) printf("\n");
}

printf("\n\n\n-------DECODED-------\n");

if ($myassembler_output !== $nasm_output) {
    $a = explode("\n", $myassembler_output);
    $b = explode("\n", $nasm_output);

    $len = min(count($a), count($b));
    for ($i = 0; $i < $len; $i++) {
        if ($a[$i] !== $b[$i]) {
            printf("%s%s%s | %s%s%s\n", COLOR_RED, $a[$i], COLOR_DEFAULT, COLOR_GREEN, $b[$i], COLOR_DEFAULT);
        } else {
            printf("%s\n", $a[$i]);
        }
    }

    printf("\n--------------------\n");

    printf("\n%s[ERROR]: DECODED BINARY ARE NOT THE SAME!%s\n", COLOR_RED, COLOR_DEFAULT);

    exit(1);
}

echo $myassembler_output;

printf("\n\n--------------------\n");

// echo $nasm_output;

printf("\n%sPASSED!%s\n\n", COLOR_GREEN, COLOR_DEFAULT);

exit(0);