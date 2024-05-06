# MiniServer

## 环境要求

- Linux
- C++20
- mysql/mariadb

## 项目运行

## 安装数据库

不同linux发行版对应的安装方法可能不同，项目应该支持mysql和mariadb.

## 安装第三方库

项目中使用cmake引入第三方库 `mysqlpp, spdlog, magic_enum`，所以需要在系统中安装这些库。

## 设置环境变量和配置数据库

项目使用环境变量保存数据库的配置信息，所以需要设置以下这些环境变量

```shell
export MINISERVER_DB_NAME='miniserver'
export MINISERVER_DB_USER='*****'
export MINISERVER_DB_PASSWORD='******'
export MINISERVER_DB_HOST='127.0.0.1'
```

```sql
// 建立miniserver数据库
create dabase miniserver;

// 创建user表
CREATE TABLE user(
    username char(50) NULL,
    password char(50) NULL
)ENGINE=InnoDB;
```

请根据自己的需要修改上面的配置项。

## 编译

在项目主目录下，使用Cmake进行编译，

```shell
cmake -B build
cmkae --build build --parallel
```

## 运行

生成的可执行文件是 `build/src/main`，执行即可

```shell
./build/src/main
```

## 连接

使用浏览器等连接到 `http://127.0.0.1:8888/`。

## 参考资料

- [@qingguoyi 实现的C++WEB服务器](https://github.com/qinguoyi/TinyWebServer)
- [@markparticle 实现的C++14WEB服务器](https://github.com/markparticle/WebServer)
- [@Codesire-Deng 项目配置模板](https://github.com/Codesire-Deng/TemplateRepoCxx)
- [memcached](https://github.com/memcached/memcached)
- Linux高性能服务器编程，游双著。
- Linux 多线程服务端编程 使用muduo C++网络库， 陈硕著。
