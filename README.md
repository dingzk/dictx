通用支持词典加载，查找，更新机制
- 已经支持kv类型词典，其他类型词典后续支持
- 支持PHP/Python/C/C++
- 支持多线程和多进程模式，多进程模式下基于共享内存，内存不会随词典变大而膨胀

> 注意：
- 多进程环境下使用了共享内存，在启动docker时候注意设置共享内存大小
```c
--shm-size="5g"
```

> 编译方式
- C/C++直接CMake
- PHP/Python直接在ext目录下执行执行make.sh 并根据提示传入相应的参数即可
- PHP的扩展请使用phpx目录下代码

> PHP使用方式如下：
- 加载dictx.so扩展，php.ini增加: 
```c++
extension=dictx.so
```
- 如果你想PHP启动就加载词典可以增加配置:

> 如果词典比较大，可能会导致php-fpm启动速度较慢，可以实际测试。（不推荐这种办法，可以见下面，直接代码按需加载）
```c++
//支持逗号分割多个具体词典
dictx.full_path_list="/data1/apache2/config/dict/all_exempt.dict"
// 支持配置确定的词典目录
dictx.directory = "/data1/apache2/config/dict/"
// 词典文件务必以.dict文件名结尾，以免误加载其他东西
```
> PHP example:
```c++
// Dictx($dict_full_path [, $load_force = 0]); //传入词典全路径，可选参数是否new的时候强制加载词典
// find($key [, $noreload = 0]); //$noreload默认为0代表会自动触发词典更新机制，传1不触发词典更新机制

// 标准用法如下：
$dict = new Dictx("/data1/apache2/config/dict/all_exempt.dict");
$b = $dict->find("1004891963"); // 没有加载过的词典这里会主动触发加载，
// 不管参数怎么传find之前自动强制加载词典(建议性锁)，已经加载过不会重复加载
var_dump($b);

// 词典内容扫描，调试时候使用
// dictx_scan();
```

> C/C++使用
- 方式比较简单，这里不在赘述了
