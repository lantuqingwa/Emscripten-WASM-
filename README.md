# ElephantEye (象眼) WASM 适配

ElephantEye 中国象棋引擎的 Emscripten/WASM 平台适配与构建脚本。
引擎原作者：Morning Yellow © 2004-2008，www.elephantbase.net，Version 3.15。
按 GNU LGPL-2.1 授权，完整许可见 `LGPL-2.1.txt`。

## 内容
- `eleeye-master/base/`：WASM 平台兼容修改（x86asm.h / base2.h / pipe.cpp）
- `eleeye-master/eleeye/`：eleeye.cpp 修改 + 新增 wasm_eleeye.cpp 导出接口
- `build_eleeye_wasm.ps1`：Emscripten 构建脚本

## 导出接口
`ee_init` / `ee_set_position` / `ee_go_depth` / `ee_go`（UCCI 风格，供移动端 / Web 内联加载）

## 构建
运行 `build_eleeye_wasm.ps1`（需 Emscripten SDK），产物为 `eleeye.js` + `eleeye.wasm`。

## 许可
LGPL-2.1。完整许可文本见 `LGPL-2.1.txt`。
