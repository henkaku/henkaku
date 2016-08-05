<?php

error_reporting(E_ALL);
ini_set('display_errors', 1);

$num_args = 7;

function array_pack(array $arr) {
	return call_user_func_array("pack", array_merge(array("L*"), $arr));
}

$args = array();
for ($i = 1; $i <= $num_args; ++$i) {
	if (!isset($_GET["a$i"])) 
		die("nope");
	$args[$i] = intval($_GET["a$i"], 16);
}

$payload = file_get_contents("stage2.bin");
$payload_u32 = array_merge(unpack("L*", $payload));
$payload_u8 = array_merge(unpack("C*", $payload));

$size_words = $payload_u32[0];

$dsize = $payload_u32[1 + 0x10/4];
$csize = $payload_u32[1 + 0x20/4];

$code_base = $args[1];
$data_base = $args[1] + $csize;

for ($i = 1; $i < $size_words; ++$i) {
	$add = 0;
	$x = $payload_u8[$size_words * 4 + 4 + $i - 1];
	if ($x == 1) {
		$add = $data_base;
	} else if ($x != 0) {
		if (!isset($args, $x))
			die("broken reloc");
		$add = $args[$x];
	}
	
	$payload_u32[$i] += $add;
}

$data = array_slice($payload_u32, 1 + 0x40 / 4, $dsize / 4);
$code = array_slice($payload_u32, 1 + (0x40 + $dsize) / 4, $csize / 4);

echo array_pack($code);
echo array_pack($data);
