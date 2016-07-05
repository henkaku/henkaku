<?php

error_reporting(E_ALL);
ini_set('display_errors', 1);

function array_pack(array $arr) {
	return call_user_func_array("pack", array_merge(array("L*"), $arr));
}

foreach (array("a", "b", "c", "d", "e", "f") as $val) {
	if (!isset($_GET[$val]))
		die("nope");
}

include("stage2-payload.php");

$dsize = $payload[0x10/4];
$csize = $payload[0x20/4];

$a = intval($_GET["a"], 16);
$b = intval($_GET["b"], 16);
$c = intval($_GET["c"], 16);
$d = intval($_GET["d"], 16);
$e = intval($_GET["e"], 16);
$f = intval($_GET["f"], 16);

$code_base = $a;
$data_base = $a + $csize;

$len = count($payload);
for ($i = 0; $i < $len; ++$i) {
	$add = 0;
	switch ($relocs[$i]) {
	case 1:
		$add = $data_base;
		break;
	case 2:
		$add = $b;
		break;
	case 3:
		$add = $c;
		break;
	case 4:
		$add = $d;
		break;
	case 5:
		$add = $e;
		break;
	case 6:
		$add = $f;
		break;
	}
	
	$payload[$i] += $add;
}

$data = array_slice($payload, 0x40 / 4, $dsize / 4);
$code = array_slice($payload, (0x40 + $dsize) / 4, $csize / 4);

echo array_pack($code);
echo array_pack($data);
