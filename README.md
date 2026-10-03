# poll-sim

選舉民調模擬器。以 C++20 建立 1,000 萬人的合成選民母體，模擬民調抽樣過程，比較簡單隨機抽樣、市話無回應與加權修正下的估計誤差。

## 專案結構

```text
poll-sim/
├── CMakeLists.txt
├── CLAUDE.md               # 給 AI 協作者的開發指引
├── docs/
│   ├── design.md           # 模型設計、參數、抽樣與估計方法、程式架構
│   ├── experiments.md      # 實驗結果與輸出欄位說明
│   └── roadmap.md          # 後續規劃
├── include/poll_sim/       # 標頭檔
└── src/                    # 實作與 main.cpp
```

## 建置

需求：CMake ≥ 3.20、支援 C++20 的編譯器（GCC ≥ 11、Clang ≥ 14 或 MSVC 2022）。

```bash
cmake -S . -B build
cmake --build build
```

預設為 Release 建置。除錯版：

```bash
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
```

## 使用

```bash
./build/poll_sim
```

程式會生成母體、印出各維度比例，再依各樣本數比較三種做法（SimpleRandom、Landline、Landline + 年齡加權）的估計結果。

參數集中在 `include/poll_sim/params.hpp`，修改後重新建置即可。各欄位意義見 [docs/experiments.md](docs/experiments.md)。
