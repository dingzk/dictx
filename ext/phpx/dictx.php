<?php
$br = (php_sapi_name() == "cli")? "":"<br>";

if(!extension_loaded('dictx')) {
	dl('dictx.' . PHP_SHLIB_SUFFIX);
}
$module = 'dictx';
$functions = get_extension_funcs($module);
echo "Functions available in the test extension:$br\n";
foreach($functions as $func) {
    echo $func."$br\n";
}


$dict = new Dictx("/foo/bar.dict");
$b = $dict->find("50000");
// $b = $dict->find("1004891963", 1); // 不触发reload机制
var_dump($b);

// dictx_scan();

echo PHP_EOL;

$i = 200;

while (true) {
    var_dump($dict->find("20000"));
}

echo PHP_EOL;
