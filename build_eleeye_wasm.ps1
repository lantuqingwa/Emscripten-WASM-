# build_eleeye_wasm.ps1 - 将 ElephantEye 3.15 编译为 WASM 模块（供 Cocos TS 侧调用）
# 用法：由本脚本所在目录运行；需先安装 Emscripten 工具链（emsdk）。
# 产物：eleeye.js + eleeye.wasm（MODULARIZE 包装，导出 ee_init/ee_set_position/ee_go_depth）

$ems = "$env:USERPROFILE\emsdk\emsdk-main"
$env:EMSDK = "C:/Users/zhihui/emsdk/emsdk-main"
$env:EMSDK_NODE = "$ems\node\24.19.0_64bit\node.exe"
$env:EMSDK_PYTHON = "$ems\python\3.13.3_64bit\python.exe"

$py = "$ems\python\3.13.3_64bit\python.exe"
$emcc = "$ems\upstream\emscripten\em++.py"
$srcDir = "C:\Users\zhihui\engine-build\eleeye_src\eleeye-master\eleeye"
$outBase = "C:\Users\zhihui\engine-build\eleeye_wasm\eleeye"

Set-Location $srcDir
New-Item -ItemType Directory -Path (Split-Path $outBase -Parent) -Force | Out-Null

$emccArgs = @(
  '-O2','-DNDEBUG','-std=c++11',
  'pregen.cpp','position.cpp','genmoves.cpp','hash.cpp','book.cpp','movesort.cpp','search.cpp','ucci.cpp','../base/pipe.cpp',
  'evaluate/evaluate.cpp','evaluate/preeval.cpp','wasm_eleeye.cpp',
  '-sEXPORTED_FUNCTIONS=_ee_init,_ee_set_position,_ee_go_depth,_ee_go,_ee_set_random,_GetEngineName,_PreEvaluate,_Evaluate',
  '-sEXPORTED_RUNTIME_METHODS=ccall,cwrap,writeArrayToMemory,lengthBytesUTF8,stringToUTF8',
  '-sMODULARIZE','-sEXPORT_NAME=createEleeyeModule',
  '-sALLOW_MEMORY_GROWTH=1',
  '-sINITIAL_MEMORY=134217728',
  '-sNO_EXIT_RUNTIME=1',
  '-o',
  ($outBase + '.js')
)

& $py $emcc @emccArgs

Write-Output "EMCC_EXIT=$LASTEXITCODE"
if (Test-Path -LiteralPath "$outBase.wasm") { Write-Output "wasm 大小: $((Get-Item -LiteralPath "$outBase.wasm").Length)" }
if (Test-Path -LiteralPath "$outBase.js") { Write-Output "js 大小: $((Get-Item -LiteralPath "$outBase.js").Length)" }
