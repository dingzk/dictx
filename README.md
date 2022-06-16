#### 通用支持词典加载，查找，更新组件
- 已经支持kv类型词典，其他类型词典后续支持，同时欢迎PR
- 支持C/C++/PHP，Python后续支持，对于脚本语言直接以内核扩展的形式支持
- 支持多线程和多进程，多进程模式下基于共享内存，内存不会随词典变大而膨胀
- 词典变更，内存会自动更新

#### 使用场景
- 大型配置（万级到千万级），变更不是很频繁，可以把数据做成词典，然后推送到服务器上，供程序加载使用
- 减少服务调用依赖，不依赖redis/mc/httpserver，直接本地加载使用
- 一般数据变更可以有几种方案：走配置中心，轻量灵活；走词典机制，性价比高；走后端服务，变更速度快，服务间解藕

#### 词典demo
```c
key\tvalue\taaaa\tbbbb\tccc\txxx\n
// 每行以\t分割，\n结尾, 查询时候传入第一列的值，以第一列的值为key进行hash查找
// 词典务必以.dict结尾
```

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
dictx.full_path_list="/foo/bar.dict"
// 支持配置确定的词典目录
dictx.directory = "/foo"
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
- 方式比较简单，查看demo，调整cmake参数，可以支持多线程
