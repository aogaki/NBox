# Phase 2: ベイズ最適化

## 概要

Optunaを使用したベイズ最適化により、He-3検出器の配置を最適化する。

## 使用方法

### 1. 環境構築

```bash
cd /Users/aogaki/WorkSpace/NBox/docs/optimization/phase2_bayesian
python3 -m venv venv
source venv/bin/activate
pip install optuna matplotlib numpy tqdm
```

### 2. 最適化実行

```bash
source venv/bin/activate

python scripts/bayesian_optimizer.py \
    --build-dir /Users/aogaki/WorkSpace/NBox/build \
    --detector-config /Users/aogaki/WorkSpace/NBox/build/eligant_tn_detector.json \
    --n-trials 50 \
    --n-events 10000 \
    --energy 1.0 \
    --energy-unit MeV \
    --n-rings 4 \
    --output results_1MeV
```

**オプション:**
- `--build-dir`: NBoxのビルドディレクトリ
- `--detector-config`: 検出器定義JSONファイル
- `--n-trials`: 最適化の試行回数
- `--n-events`: シミュレーションごとのイベント数
- `--energy`: 中性子エネルギー
- `--energy-unit`: エネルギー単位（eV, keV, MeV）
- `--source-file`: エネルギースペクトルファイル（オプション、指定時はenergy無視）
- `--n-rings`: 検出器リング数（2, 3, 4）
- `--output`: 出力ディレクトリ
- `--n-short`: 短い検出器の本数（デフォルト: 28）
- `--n-long`: 長い検出器の本数（デフォルト: 40）
- `--short-type`: 短い検出器のタイプ名（デフォルト: He3_ELIGANT）
- `--long-type`: 長い検出器のタイプ名（デフォルト: He3_ELIGANT_Long）

### Cf-252スペクトルでの最適化

```bash
python scripts/bayesian_optimizer.py \
    --build-dir /Users/aogaki/WorkSpace/NBox/build \
    --detector-config /Users/aogaki/WorkSpace/NBox/build/eligant_tn_detector.json \
    --source-file /Users/aogaki/WorkSpace/NBox/build/cf252_source.root \
    --n-trials 50 \
    --n-events 10000 \
    --output results_cf252
```

## 出力ファイル

### 最適化結果

- `best_parameters.json` - 最適パラメータと効率
- `best_geometry.json` - 最適配置のジオメトリファイル（NBoxで直接使用可能）
- `optimization_history.json` - 全試行の履歴

### ジオメトリ設定

`configs/trial_XXXX.json` - 各試行のジオメトリ設定

## 最適化パラメータ

| パラメータ | 型 | 範囲 | 説明 |
|-----------|-----|------|------|
| r1 | float | [35, 70] | リングA半径 [mm] |
| r2 | float | [r1+31, 120] | リングB半径 [mm] |
| r3 | float | [r2+31, 170] | リングC半径 [mm] |
| r4 | float | [r3+31, 217] | リングD半径 [mm] |
| n1 | int | [4, 12] | リングA総本数 |
| n2 | int | [8, 20] | リングB総本数 |
| n3 | int | [12, 24] | リングC総本数 |
| n4 | int | [16, 28] | リングD総本数 |
| s1 | int | [0, n1] | リングA短検出器本数 |
| s2 | int | [0, n2] | リングB短検出器本数 |
| s3 | int | [0, n3] | リングC短検出器本数 |
| s4 | int | [0, n4] | リングD短検出器本数 |

## 検出器インベントリ

| タイプ | 長さ | デフォルト本数 |
|--------|------|----------------|
| He3_ELIGANT (短) | 500 mm | 28 |
| He3_ELIGANT_Long (長) | 1000 mm | 40 |

各リングの総本数 (n) のうち、短検出器本数 (s) を最適化します。残り (n-s) は長検出器となります。
短検出器・長検出器の総使用数はインベントリ制約を超えることはできません。

## 物理的制約

- 検出器直径: 25.4 mm
- 最小間隔: 5 mm
- リング間最小距離: 30.4 mm
- ビームパイプ半径: 22 mm
- モデレータ半幅: 230 mm

## 実行結果例

### 1 MeV中性子、20試行

```
Best efficiency: 76.87%
Best radii: [50.1, 104.9, 143.8, 178.6] mm
Best counts: [10, 18, 19, 18]
Best short counts: [4, 8, 10, 6]
Total detectors: 65 (28 short + 37 long)
```

## スクリプト

- `bayesian_optimizer.py` - メイン最適化スクリプト
- `geometry_generator.py` - ジオメトリJSON生成
- `run_nbox.py` - NBox実行・効率計算ラッパー

## 注意事項

- TChainを使用してスレッドファイルを直接読み込むため、haddは不要
- ROOTは`/opt/ROOT/bin/root`を直接使用（subprocessでのsource問題を回避）
