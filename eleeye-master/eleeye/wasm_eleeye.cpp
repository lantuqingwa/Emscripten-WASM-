// wasm_eleeye.cpp - ElephantEye WASM C API 封装（供 Cocos TS 侧调用）
//
// 仅为 WASM 目标暴露三个函数：ee_init / ee_set_position / ee_go_depth，
// 直接驱动核心搜索（SearchMain），不走 STDIN/STDOUT 管道；
// 走法结果由 SearchMain 内部 printf("bestmove ...") 输出，经 Emscripten
// 的 Module.print 回调捕获并解析。无需重编 WASM 即可通过 go depth 阶梯调速。
#include <stdio.h>
#include "../base/base2.h"
#include "../base/parse.h"
#include "ucci.h"
#include "pregen.h"
#include "position.h"
#include "hash.h"
#include "search.h"

#ifdef __EMSCRIPTEN__
/* 静态链接 evaluate/preeval 的内置评估（与 eleeye.cpp 的 WASM 适配一致） */
extern "C" const char *GetEngineName(void);
extern "C" void PreEvaluate(PositionStruct *lppos, PreEvalStruct *lpPreEval);
extern "C" int Evaluate(const PositionStruct *lppos, int vlAlpha, int vlBeta);
#endif

// 中断计数（原定义于 eleeye.cpp，此处为 wrapper 自备）
const int INTERRUPT_COUNT = 4096;

extern "C" {

// 初始化引擎（等价于 UCCI 握手后的内部状态）
void ee_init(void) {
  // 先绑定评估函数指针（main 中由 LoadEvalApi 完成，此处须在 PreEvaluate 调用前设置）
  Search.GetEngineName = GetEngineName;
  Search.PreEvaluate = PreEvaluate;
  Search.Evaluate = Evaluate;
  PreGenInit();
  NewHash(24); // 24 = 16MB 哈希表
  Search.pos.FromFen(cszStartFen);
  Search.pos.nDistance = 0;
  Search.PreEvaluate(&Search.pos, &PreEval);
  Search.nBanMoves = 0;
  Search.bQuit = Search.bBatch = Search.bDebug = Search.bAlwaysCheck = false;
  Search.bUseHash = Search.bUseBook = Search.bNullMove = Search.bKnowledge = true;
  Search.bIdle = false;
  Search.nCountMask = INTERRUPT_COUNT - 1;
  Search.nRandomMask = 0;
  Search.rc4Random.InitRand();
}

// 设置盘面（FEN 字符串），等价于 UCCI 的 "position fen ..."
void ee_set_position(const char *fen) {
  Search.pos.FromFen(fen);
  Search.pos.nDistance = 0;
  Search.PreEvaluate(&Search.pos, &PreEval);
  Search.nBanMoves = 0;
}

// 按固定深度搜索（等价于 UCCI 的 "go depth N"），结果经 stdout 输出
void ee_go_depth(int depth) {
  Search.nGoMode = GO_MODE_INFINITY;
  Search.nNodes = 0;
  SearchMain(depth);
}

// 限时搜索：movetime > 0 时按毫秒中断（GO_MODE_TIMER，nProperTimer=nMaxTimer=movetime），
// 否则等价于 ee_go_depth。depth 作为迭代加深最大深度上限，time 主导中断。
void ee_go(int depth, int movetime) {
  if (movetime > 0) {
    Search.nGoMode = GO_MODE_TIMER;
    Search.nNodes = 0;
    Search.nProperTimer = movetime;
    Search.nMaxTimer = movetime;
    SearchMain(depth > 0 ? depth : 1);
  } else {
    Search.nGoMode = GO_MODE_INFINITY;
    Search.nNodes = 0;
    SearchMain(depth);
  }
}

// 设置引擎随机性（基于深搜评分的"次优"选择）：level 0 = 无随机（最强棋力）。
// level 越大，根选择时对最优分施加的随机扰动（Search.nRandomMask）越大，
// 越可能走出"评分接近的次优着"。次优着由引擎自身深搜保证质量：弱而合理、不送子。
// 映射：0->0, 1->1, 2->3, 3->7, 4->15, 5->31, 6->63
void ee_set_random(int level) {
  Search.nRandomMask = level <= 0 ? 0 : ((1 << level) - 1);
}

}
