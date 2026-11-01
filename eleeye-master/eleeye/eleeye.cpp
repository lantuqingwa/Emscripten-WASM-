/*
eleeye.cpp - Source Code for ElephantEye, Part IX

ElephantEye - a Chinese Chess Program (UCCI Engine)
Designed by Morning Yellow, Version: 3.15, Last Modified: Jul. 2008
Copyright (C) 2004-2008 www.elephantbase.net

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <stdio.h>
#include "../base/base2.h"
#include "../base/parse.h"
#include "ucci.h"
#include "pregen.h"
#include "position.h"
#include "hash.h"
#include "search.h"

#ifdef __EMSCRIPTEN__
/* WASM 适配：静态链接 evaluate/preeval 的内置评估，替代 dlopen */
extern "C" const char *GetEngineName(void);
extern "C" void PreEvaluate(PositionStruct *lppos, PreEvalStruct *lpPreEval);
extern "C" int Evaluate(const PositionStruct *lppos, int vlAlpha, int vlBeta);
#endif

#ifdef _WIN32
  #include <windows.h>
  const char *const cszLibEvalFile = "EVALUATE.DLL";
#else
  #include <dlfcn.h>
  #define WINAPI
  const char *const cszLibEvalFile = "libeval.so";
#endif

const int INTERRUPT_COUNT = 4096; // �������ɽ�������ж�

#ifndef __EMSCRIPTEN__
static const char *WINAPI GetEngineName(void) {
  return NULL;
}

static void WINAPI PreEvaluate(PositionStruct *lppos, PreEvalStruct *lpPreEval) {
  // ȱʡ�ľ���Ԥ���۹��̣�ʲô������
}

static int WINAPI Evaluate(const PositionStruct *lppos, int vlAlpha, int vlBeta) {
  // ȱʡ�ľ������۹��̣�ֻ����������ֵ
  return lppos->Material();
}
#endif

#ifdef _WIN32

inline HMODULE LoadEvalApi(const char *szLibEvalFile) {
  HMODULE hModule;  
  hModule = LoadLibrary(szLibEvalFile);
  if (hModule == NULL) {
    Search.GetEngineName = GetEngineName;
    Search.PreEvaluate = PreEvaluate;
    Search.Evaluate = Evaluate;
  } else {
    Search.GetEngineName = (const char *(WINAPI *)(void)) GetProcAddress(hModule, "_GetEngineName@0");
    Search.PreEvaluate = (void (WINAPI *)(PositionStruct *, PreEvalStruct *)) GetProcAddress(hModule, "_PreEvaluate@8");
    Search.Evaluate = (int (WINAPI *)(const PositionStruct *, int, int)) GetProcAddress(hModule, "_Evaluate@12");
  }
  return hModule;
}

inline void FreeEvalApi(HMODULE hModule) {
  if (hModule != NULL) {
    FreeLibrary(hModule);
  }
}

#else

inline void *LoadEvalApi(const char *szLibEvalFile) {
#ifdef __EMSCRIPTEN__
  (void) szLibEvalFile;
  /* WASM：静态链接内置评估，不动态加载 */
  Search.GetEngineName = GetEngineName;
  Search.PreEvaluate = PreEvaluate;
  Search.Evaluate = Evaluate;
  return NULL;
#else
  void *hModule;
  hModule = dlopen(szLibEvalFile, RTLD_LAZY);
  if (hModule == NULL) {
    Search.GetEngineName = GetEngineName;
    Search.PreEvaluate = PreEvaluate;
    Search.Evaluate = Evaluate;
  } else {
    Search.GetEngineName = (const char *(*)(void)) dlsym(hModule, "GetEngineName");
    Search.PreEvaluate = (void (*)(PositionStruct *, PreEvalStruct *)) dlsym(hModule, "PreEvaluate");
    Search.Evaluate = (int (*)(const PositionStruct *, int, int)) dlsym(hModule, "Evaluate");
  }
  return hModule;
#endif
}

inline void FreeEvalApi(void *hModule) {
#ifdef __EMSCRIPTEN__
  (void) hModule;
#else
  if (hModule != NULL) {
    dlclose(hModule);
  }
#endif
}

#endif

inline void PrintLn(const char *sz) {
  printf("%s\n", sz);
  fflush(stdout);
}

int main(void) {
  int i;
  bool bPonderTime;
  UcciCommStruct UcciComm;
  char szLibEvalFile[1024];
  const char *szEngineName;
  PositionStruct posProbe;
#ifdef _WIN32
  HMODULE hModule;  
#else
  void *hModule;
#endif

  if (BootLine() != UCCI_COMM_UCCI) {
    return 0;
  }
  LocatePath(Search.szBookFile, "BOOK.DAT");
  LocatePath(szLibEvalFile, cszLibEvalFile);
  hModule = LoadEvalApi(szLibEvalFile);
  bPonderTime = false;
  PreGenInit();
  NewHash(24); // 24=16MB, 25=32MB, 26=64MB, ...
  Search.pos.FromFen("rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1");
  Search.pos.nDistance = 0;
  Search.PreEvaluate(&Search.pos, &PreEval);
  Search.nBanMoves = 0;
  Search.bQuit = Search.bBatch = Search.bDebug = Search.bAlwaysCheck = false;
  Search.bUseHash = Search.bUseBook = Search.bNullMove = Search.bKnowledge = true;
  Search.bIdle = false;
  Search.nCountMask = INTERRUPT_COUNT - 1;
  Search.nRandomMask = 0;
  Search.rc4Random.InitRand();
  szEngineName = Search.GetEngineName();
  if (szEngineName == NULL) {
    PrintLn("id name ElephantEye");
  } else {
    printf("id name %s / ElephantEye\n", szEngineName);
    fflush(stdout);
  }
  PrintLn("id version 3.15");
  PrintLn("id copyright 2004-2008 www.elephantbase.net");
  PrintLn("id author Morning Yellow");
  PrintLn("id user ElephantEye Test Team");
  PrintLn("option usemillisec type check default true");
  PrintLn("option promotion type check default false");
  PrintLn("option batch type check default false");
  PrintLn("option debug type check default false");
  PrintLn("option ponder type check default false");
  PrintLn("option alwayscheck type check default false");
  PrintLn("option usehash type check default true");
  PrintLn("option usebook type check default true");
  printf("option bookfiles type string default %s\n", Search.szBookFile);
  fflush(stdout);
  printf("option evalapi type string default %s\n", szLibEvalFile);
  fflush(stdout);
  PrintLn("option hashsize type spin min 16 max 1024 default 16");
  PrintLn("option idle type combo var none var small var medium var large default none");
  PrintLn("option pruning type combo var none var small var medium var large default large");
  PrintLn("option knowledge type combo var none var small var medium var large default large");
  PrintLn("option randomness type combo var none var small var medium var large default none");
  PrintLn("option newgame type button");
  PrintLn("ucciok");

  // �����ǽ���ָ����ṩ�Բߵ�ѭ����
  while (!Search.bQuit) {
    switch (IdleLine(UcciComm, Search.bDebug)) {
    case UCCI_COMM_ISREADY:
      PrintLn("readyok");
      break;
    case UCCI_COMM_STOP:
      PrintLn("nobestmove");
      break;
    case UCCI_COMM_POSITION:
      BuildPos(Search.pos, UcciComm);
      Search.pos.nDistance = 0;
      Search.PreEvaluate(&Search.pos, &PreEval);
      Search.nBanMoves = 0;
      break;
    case UCCI_COMM_BANMOVES:
      Search.nBanMoves = UcciComm.nBanMoveNum;
      for (i = 0; i < UcciComm.nBanMoveNum; i ++) {
        Search.wmvBanList[i] = COORD_MOVE(UcciComm.lpdwBanMovesCoord[i]);
      }
      break;
    case UCCI_COMM_SETOPTION:
      switch (UcciComm.Option) {
      case UCCI_OPTION_PROMOTION:
        PreEval.bPromotion = UcciComm.bCheck;
        break;
      case UCCI_OPTION_BATCH:
        Search.bBatch = UcciComm.bCheck;
        break;
      case UCCI_OPTION_DEBUG:
        Search.bDebug = UcciComm.bCheck;
        break;
      case UCCI_OPTION_PONDER:
        bPonderTime = UcciComm.bCheck;
        break;
      case UCCI_OPTION_ALWAYSCHECK:
        Search.bAlwaysCheck = UcciComm.bCheck;
        break;
      case UCCI_OPTION_USEHASH:
        Search.bUseHash = UcciComm.bCheck;
        break;
      case UCCI_OPTION_USEBOOK:
        Search.bUseBook = UcciComm.bCheck;
        break;
      case UCCI_OPTION_BOOKFILES:
        if (AbsolutePath(UcciComm.szOption)) {
          strcpy(Search.szBookFile, UcciComm.szOption);
        } else {
          LocatePath(Search.szBookFile, UcciComm.szOption);
        }
        break;
      case UCCI_OPTION_EVALAPI:
        if (AbsolutePath(UcciComm.szOption)) {
          strcpy(szLibEvalFile, UcciComm.szOption);
        } else {
          LocatePath(szLibEvalFile, UcciComm.szOption);
        }
        FreeEvalApi(hModule);
        hModule = LoadEvalApi(szLibEvalFile);
        break;
      case UCCI_OPTION_HASHSIZE:
        DelHash();
        i = 19; // С��1������0.5M�û���
        while (UcciComm.nSpin > 0) {
          UcciComm.nSpin /= 2;
          i ++;
        }
        NewHash(MAX(i, 24)); // ��С���û�����Ϊ16M
        break;
      case UCCI_OPTION_IDLE:
        switch (UcciComm.Grade) {
        case UCCI_GRADE_NONE:
          Search.bIdle = false;
          Search.nCountMask = INTERRUPT_COUNT - 1;
          break;
        case UCCI_GRADE_SMALL:
          Search.bIdle = true;
          Search.nCountMask = INTERRUPT_COUNT / 4 - 1;
          break;
        case UCCI_GRADE_MEDIUM:
          Search.bIdle = true;
          Search.nCountMask = INTERRUPT_COUNT / 16 - 1;
          break;
        case UCCI_GRADE_LARGE:
          Search.bIdle = true;
          Search.nCountMask = INTERRUPT_COUNT / 64 - 1;
          break;
        default:
          break;
        }
        break;
      case UCCI_OPTION_PRUNING:
        Search.bNullMove = (UcciComm.Grade != UCCI_GRADE_NONE);
        break;
      case UCCI_OPTION_KNOWLEDGE:
        Search.bKnowledge = (UcciComm.Grade != UCCI_GRADE_NONE);
        break;
      case UCCI_OPTION_RANDOMNESS:
        switch (UcciComm.Grade) {
        case UCCI_GRADE_NONE:
          Search.nRandomMask = 0;
          break;
        case UCCI_GRADE_SMALL:
          Search.nRandomMask = 1;
          break;
        case UCCI_GRADE_MEDIUM:
          Search.nRandomMask = 3;
          break;
        case UCCI_GRADE_LARGE:
          Search.nRandomMask = 7;
          break;
        default:
          break;
        }
        break;
      default:
        break;
      }
      break;
    case UCCI_COMM_GO:
      Search.bPonder = UcciComm.bPonder;
      Search.bDraw = UcciComm.bDraw;
      switch (UcciComm.Go) {
      case UCCI_GO_DEPTH:
        Search.nGoMode = GO_MODE_INFINITY;
        Search.nNodes = 0;
        SearchMain(UcciComm.nDepth);
        break;
      case UCCI_GO_NODES:
        Search.nGoMode = GO_MODE_NODES;
        Search.nNodes = UcciComm.nNodes;
        SearchMain(UCCI_MAX_DEPTH);
        break;
      case UCCI_GO_TIME_MOVESTOGO:
      case UCCI_GO_TIME_INCREMENT:
        Search.nGoMode = GO_MODE_TIMER;
        if (UcciComm.Go == UCCI_GO_TIME_MOVESTOGO) {
          // ����ʱ���ƣ���ʣ��ʱ��ƽ�����䵽ÿһ������Ϊ�ʵ�ʱ�ޡ�
          // ʣ�ಽ����1��5�����ʱ��������ʣ��ʱ���100%��90%��80%��70%��60%��5���϶���50%
          Search.nProperTimer = UcciComm.nTime / UcciComm.nMovesToGo;
          Search.nMaxTimer = UcciComm.nTime * MAX(5, 11 - UcciComm.nMovesToGo) / 10;
        } else {
          // ���ڼ�ʱ�ƣ�������ֻ���20�غ��ڽ��������ƽ��ÿһ�����ʵ�ʱ�ޣ����ʱ����ʣ��ʱ���һ��
          Search.nProperTimer = UcciComm.nTime / 20 + UcciComm.nIncrement;
          Search.nMaxTimer = UcciComm.nTime / 2;
        }
        // ����Ǻ�̨˼����ʱ�������ԣ���ô�ʵ�ʱ����Ϊԭ����1.25��
        Search.nProperTimer += (bPonderTime ? Search.nProperTimer / 4 : 0);
        Search.nMaxTimer = MIN(Search.nMaxTimer, Search.nProperTimer * 10);
        SearchMain(UCCI_MAX_DEPTH);
        break;
      default:
        break;
      }
      break;
    case UCCI_COMM_PROBE:
      BuildPos(posProbe, UcciComm);
      if (!PopHash(posProbe)) {
        PopLeaf(posProbe);
      }
      break;
    case UCCI_COMM_QUIT:
      Search.bQuit = true;
      break;
    default:
      break;
    }
  }
  DelHash();
  FreeEvalApi(hModule);
  PrintLn("bye");
  return 0;
}
