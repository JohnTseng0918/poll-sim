# poll-sim

選舉民調模擬器。以 C++20 建立 1,000 萬人的合成選民母體，模擬民調抽樣過程，比較簡單隨機抽樣、市話與手機調查的無回應（MAR 與 MNAR）及多重性偏差，以及加權修正下的估計誤差。

## 專案結構

```text
poll-sim/
├── CMakeLists.txt
├── CLAUDE.md               # 給 AI 協作者的開發指引
├── docs/
│   ├── concepts.md         # 抽樣方法、加權與統計名詞介紹
│   ├── design.md           # 模型設計、參數、抽樣與估計方法、程式架構
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

程式會生成母體、印出各維度比例與電話涵蓋，再依各樣本數比較各種做法的估計結果：SimpleRandom、Landline（MAR 與 MNAR 接聽率，各自有無年齡加權）、Mobile（未加權、多重性加權、多重性 + 年齡加權）。

參數集中在 `include/poll_sim/params.hpp`，修改後重新建置即可。結果為固定 seed，可完全重現。

### 輸出欄位說明

| 欄位 | 意義 |
|---|---|
| Mean | 各次民調估計值的平均 |
| Bias | Mean − 真值 |
| MoE | 1.96 × 各次估計值的實際標準差 |
| Coverage | 95% 信賴區間（估計值 ± 1.96 SE）包含真值的比例 |
| Within3 | 估計值落在真值 ±3 個百分點內的比例 |
| Young | 平均年輕受訪者人數 |
| Eff. n | Kish 有效樣本數 (Σw)² / Σw² |
| Deff | 設計效應 n / Eff. n |
| EmptyAge | 有年齡層完全沒有受訪者的民調比例 |
| Mult Eff. n | 多重性加權後的 Kish 有效樣本數 |
| +Age Eff. n | 多重性 + 年齡加權後的 Kish 有效樣本數 |
| Dup | 每次民調中同一人經由不同號碼重複受訪的平均次數 |
