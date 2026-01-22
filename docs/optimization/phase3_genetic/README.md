# Phase 3: 遺伝的アルゴリズム最適化

## 概要

DEAPライブラリを使用した遺伝的アルゴリズムによるHe-3検出器配置の最適化。

## 制約条件

- 全検出器を使用: 28本（短）+ 40本（長）= 68本
- 各リングは同一タイプの検出器のみ使用

## 使用方法

### 1. 環境構築

Phase 2のvenvを使用:

```bash
cd /Users/aogaki/WorkSpace/NBox/docs/optimization/phase2_bayesian
source venv/bin/activate
pip install deap
```

### 2. 最適化実行

```bash
python /Users/aogaki/WorkSpace/NBox/docs/optimization/phase3_genetic/genetic_optimizer.py \
    --build-dir /Users/aogaki/WorkSpace/NBox/build \
    --detector-config /Users/aogaki/WorkSpace/NBox/build/eligant_tn_detector.json \
    --n-generations 20 \
    --population 20 \
    --n-events 10000 \
    --energy 1.0 \
    --energy-unit MeV \
    --n-rings 4 \
    --output results_1MeV
```

**オプション:**
- `--build-dir`: NBoxのビルドディレクトリ
- `--detector-config`: 検出器定義JSONファイル
- `--n-generations`: 世代数（デフォルト: 20）
- `--population`: 個体数（デフォルト: 20）
- `--n-events`: シミュレーションごとのイベント数
- `--energy`: 中性子エネルギー
- `--energy-unit`: エネルギー単位
- `--n-rings`: リング数（2, 3, 4）
- `--output`: 出力ディレクトリ

## 出力ファイル

- `best_parameters.json` - 最適パラメータ
- `best_geometry.json` - 最適配置のジオメトリファイル
- `optimization_history.json` - 全評価の履歴
- `generation_stats.json` - 世代ごとの統計

## アルゴリズム詳細

### 個体表現

各個体は以下のパラメータで構成:
- `r1, r2, r3, r4`: リング半径 [mm]
- `type1, type2, type3, type4`: リングタイプ (0=short, 1=long)
- `n1, n2, n3, n4`: 各リングの検出器本数

### 遺伝的演算

- **選択**: トーナメント選択 (tournsize=3)
- **交叉**: 半径の部分交換 + タイプスワップ (確率70%)
- **突然変異**: 半径のガウスノイズ + タイプスワップ (確率30%)
- **修復**: 制約を満たすように個体を修復

## 実行結果例

### 1 MeV中性子、15世代×15個体

```
Best efficiency: 80.04%
Best radii: [50.0, 100.0, 144.4, 200.0] mm
Ring types: ['short', 'long', 'short', 'long']
Counts: [10, 20, 18, 20]
Total detectors: 68 (28 short + 40 long)
Total evaluations: 185
```

## ベイズ最適化との比較

| 項目 | ベイズ最適化 | 遺伝的アルゴリズム |
|------|-------------|-------------------|
| 最適効率 | 81.99% | 80.04% |
| 評価回数 | 50 | 185 |
| 収束速度 | 速い | 遅い |
| 探索範囲 | 局所的 | 大域的 |

ベイズ最適化は少ない評価回数で高い効率を達成。遺伝的アルゴリズムは探索範囲が広いが、収束に時間がかかる。
