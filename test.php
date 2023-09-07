<?php

const COLOR_DEFAULT = "\033[0m";
const COLOR_RED     = "\033[0;31m";
const COLOR_GREEN   = "\033[0;32m";

function pout(array $output) {
    global $argv;
    if (@$argv[1] === '-q') return '';
    return implode("\n", $output); 
}

exec('call .\build.bat', $out, $code);
if ($code !== 0) { 
    echo pout($out); 
    printf("\n\n%s[ERROR]: C compilation is failed!%s\n", COLOR_RED, COLOR_DEFAULT);
    exit(1); 
}

exec('call .\build\assembler.exe', $out, $code);
echo pout($out);

exec('nasm.exe mock\listing_0039_more_movs.asm', $out, $code);
if ($code !== 0) { 
    echo pout($out); 
    printf("\n\n%s[ERROR]: NASM compilation is failed!%s\n", COLOR_RED, COLOR_DEFAULT);
    exit(1); 
}

exec('call .\build\main.exe .\mock\a.out --decode --hide_inst_mem_addr', $myassembler_output, $code);
$myassembler_output = pout($myassembler_output);
echo $myassembler_output;

exec('call .\build\main.exe .\mock\listing_0039_more_movs --decode --hide_inst_mem_addr', $nasm_output, $code);
$nasm_output = pout($nasm_output);
echo $nasm_output;

// print("\n\nmyssembler:".md5($myassembler_output) . "\nnasm:". md5($nasm_output));
// exit;

if ($myassembler_output !== $nasm_output) {
    printf("\n\n%s[ERROR]: DECODED BINARY ARE NOT THE SAME!%s\n", COLOR_RED, COLOR_DEFAULT);
    exit(1);
}

printf("\n\n%sPASSED!%s\n\n", COLOR_GREEN, COLOR_DEFAULT);

exit(0);