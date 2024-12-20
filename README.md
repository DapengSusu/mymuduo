# MyMuduo

MyMuduo is a simple and efficient C++ network library based on the Reactor pattern. It is designed to be easy to use and integrate into your projects.

## Features

- Non-blocking I/O
- Event-driven architecture
- Lightweight and efficient
- Easy to integrate

## Getting Started

### Prerequisites

- C++11 or later
- CMake 3.10 or later

### Building

To build MyMuduo, follow these steps:

```sh
git clone https://github.com/yourusername/mymuduo.git
cd mymuduo

# 编译源码并拷贝库到系统中
# 默认头文件路径：/usr/include/mymuduo
# 默认库路径：/usr/lib
sudo ./autobuild.sh

# 手动编译测试服务器
cd example && make
./testserver

# 快速编译修改启动测试服务器（必须执行过cmake）
sudo ./quicktest.sh
```

### Usage

Include MyMuduo in your project and start using it to handle network events efficiently.

## Contributing

Contributions are welcome! Please fork the repository and submit a pull request.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Inspired by the Muduo network library
- Thanks to all contributors and users
