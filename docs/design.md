# 模型設計

## 目標

1. **理論隨機抽樣（SRS）**：驗證理論誤差範圍（±3%）。
2. **系統性偏差**：模擬 Coverage Bias / Non-response Bias，特別是不同年齡層接聽率差異造成的統計失真。
3. **加權還原（Raking / IPF）**：量化加權的修正極限與權重爆炸現象。
4. **平行運算**：為大規模蒙地卡羅敏感度分析與多執行緒（Thread-pool / TBB / OpenMP）預留架構。

## 母體設定（第一階段）

為避免「假精緻」與維度詛咒，第一階段只使用三個人口統計維度：年齡、地區、性別。三者假設彼此獨立。

**年齡層**

| 年齡層 | 母體佔比 | 支持 DPP | 支持 KMT | 不表態 | 市話接聽率 |
|---|---|---|---|---|---|
| Young (20–39) | 30% | 65% | 25% | 10% | 5% |
| Middle (40–59) | 35% | 45% | 40% | 15% | 30% |
| Senior (60+) | 35% | 30% | 55% | 15% | 60% |

支持 KMT 與不表態的比例為暫定值。

**其他維度**

- **Region**：North 45%、Central 25%、South 27%、East 3%
- **Gender**：Male 49%、Female 51%

目前投票傾向只與年齡有關，地區與性別不影響投票傾向。

**接聽率**以 `[年齡][候選人]` 為索引：

- 同一年齡層各候選人的接聽率相同時，接聽只與年齡有關（MAR），依年齡加權即可修正偏差。
- 接聽率依投票意向而不同時（MNAR），加權無法完全修正，可用來觀察加權的極限。

所有參數集中在 `include/poll_sim/params.hpp`：

- `PopulationParams`：母體大小、各維度比例、投票傾向、接聽率、母體 seed。
- `SimParams`：要測試的樣本數清單 `sample_sizes`、重複次數 `replications`、抽樣 seed。

母體與抽樣使用不同的 seed，可以固定母體、只改變抽樣。

## 模擬流程

1. 依 `PopulationParams` 生成合成選民母體。
2. 從母體實際計算各候選人的真實支持率與年齡比例，作為比較基準（不使用參數的理論值）。
3. 對每個樣本數 n，各抽樣方法重複執行 `replications` 次民調。
4. 每次民調以不同估計方法算出支持率與標準誤，彙整成統計表。

同一抽樣方法的各種估計方法共用同一批民調樣本，差異只來自估計方法。

## 抽樣方法

| 方法 | 說明 |
|---|---|
| SimpleRandom | 不重複隨機抽出 n 人，抽到重複就重抽；每個人都回答（理想情況） |
| Landline | 不斷隨機撥打未撥過的選民，每人依 `response_rate[年齡][候選人]` 決定是否接聽，直到湊滿 n 位受訪者；拒接者不會再被撥打 |

## 估計方法

| 方法 | 估計值 | 標準誤 |
|---|---|---|
| 未加權 | 樣本中的支持比例 p | √(p(1−p)/n) |
| 年齡加權（post-stratification） | Σₐ Wₐ · pₐ | √(Σₐ Wₐ² · pₐ(1−pₐ)/nₐ) |

- Wₐ：母體中年齡層 a 的比例；pₐ、nₐ：樣本中年齡層 a 的支持比例與人數。
- 某年齡層沒有受訪者時，略過該層，並將其餘年齡層的 Wₐ 重新正規化。
- Kish 有效樣本數：每位年齡層 a 的受訪者權重為 Wₐ/nₐ，有效樣本數 = (Σw)²/Σw² = 1 / Σₐ(Wₐ²/nₐ)。

## 程式架構

| 模組 | 職責 |
|---|---|
| `types` | `AgeGroup`、`Region`、`Gender`、`Candidate` 列舉與 `to_string` |
| `voter` | 選民結構 `Voter` |
| `params` | `PopulationParams`、`SimParams` 與參數驗證 |
| `population` | `generate_population`：依參數生成母體 |
| `sampling` | `SamplingMethod`、`simple_random_sample`、`landline_sample` |
| `poll` | `PollResult`（各候選人、年齡層、年齡 × 候選人人數）、`tally`、母體真值計算 |
| `experiment` | `run_experiment`：以指定抽樣方法重複執行民調 |
| `estimate` | `Estimate`、未加權與年齡加權估計、Kish 有效樣本數 |
| `stats` | `summarize`：彙整多次估計的偏差、誤差範圍、涵蓋率等 |
| `main.cpp` | 串接上述模組並印出結果 |
