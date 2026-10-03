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

`CMakePresets.json` 提供三種建置版本，各自建在不同目錄：

| Preset | Build type | 目錄 | 用途 |
|---|---|---|---|
| `release` | Release | `build/` | 一般執行（`-O3`） |
| `profile` | Profile | `build-prof/` | 效能分析：與 Release 相同的最佳化，加上除錯資訊與 frame pointer（`-g -fno-omit-frame-pointer`），供 `perf record -g` 使用 |
| `debug` | Debug | `build-debug/` | 除錯（不最佳化） |

```bash
cmake --preset release && cmake --build --preset release
cmake --preset profile && cmake --build --preset profile
```

不使用 preset 時，以 `CMAKE_BUILD_TYPE` 指定，未指定則為 Release：

```bash
cmake -S . -B build-prof -DCMAKE_BUILD_TYPE=Profile
cmake --build build-prof
```

效能分析範例（Profile build 目前只支援 GCC / Clang）：

```bash
perf stat -e cycles,instructions,cache-misses,branch-misses ./build-prof/poll_sim
perf record -g ./build-prof/poll_sim && perf report --stdio
```

## 使用

```bash
./build/poll_sim                      # 隨機抽取（預設）
./build/poll_sim --order=sequential   # 從隨機起點依序抽取，統計上等價、速度快很多
./build/poll_sim --help
```

程式會生成母體、印出各維度比例與電話涵蓋，再依各樣本數比較各種做法的估計結果：SimpleRandom、Landline（MAR 與 MNAR 接聽率，各自有無年齡加權）、Mobile（未加權、多重性加權、多重性 + 年齡加權）。

參數集中在 `include/poll_sim/params.hpp`，修改後重新建置即可；抽取順序也可以用 `--order` 在執行時切換。結果為固定 seed，可完全重現。

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
