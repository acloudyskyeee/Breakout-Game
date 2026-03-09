# C++ 图形版打砖块小游戏（SDL2）

这是一个使用 **C++ + SDL2** 实现的窗口图形版打砖块小游戏。

## 特性

- 图形窗口渲染（非终端字符画）
- 键盘实时控制挡板
- 小球与墙壁 / 挡板 / 砖块碰撞
- 分数与生命值统计（显示在窗口标题）
- 胜利与失败状态（可按 `R` 重开）

## 依赖

- SDL2 开发库

Ubuntu/Debian 可安装：

```bash
sudo apt-get update
sudo apt-get install -y libsdl2-dev
```

## 编译

```bash
g++ -std=c++17 -O2 breakout.cpp -o breakout `sdl2-config --cflags --libs`
```

## 运行

```bash
./breakout
```

## 操作

- `A` / `←`：向左移动挡板
- `D` / `→`：向右移动挡板
- `Esc`：退出游戏
- `R`：游戏结束后重新开始

## 代码讲解文档

- 详见 `SDL2_代码讲解.md`（面向 SDL2 初学者，包含项目结构和逐模块说明）。
