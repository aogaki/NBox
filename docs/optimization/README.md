# NBox 検出器配置最適化マニュアル

He-3検出器の配置を最適化するための包括的なガイドです。

## 目次

1. [概要](#概要)
2. [環境構築](#環境構築)
3. [Phase 1: フラックスマップ解析](#phase-1-フラックスマップ解析)
4. [Phase 2: ベイズ最適化](#phase-2-ベイズ最適化)
5. [Phase 3: 遺伝的アルゴリズム](#phase-3-遺伝的アルゴリズム)
6. [結果の比較](#結果の比較)
7. [トラブルシューティング](#トラブルシューティング)

---

## 概要

### 最適化の目標

中性子検出効率を最大化する検出器配置を見つける。

### 最適化パラメータ

| パラメータ | 説明 |
|-----------|------|
| r1, r2, r3, r4 | 各リングの半径 [mm] |
| リングタイプ | 各リングの検出器タイプ（short/long） |
| 検出器本数 | 各リングの検出器本数 |

### 制約条件

| 制約 | 値 |
|------|-----|
| 検出器直径 | 25.4 mm |
| 最小間隔 | 5 mm |
| リング間最小距離 | 30.4 mm |
| ビームパイプ半径 | 22 mm |
| モデレータサイズ | 1000 x 1000 x 1000 mm |
| 短検出器（He3_ELIGANT） | 28本 |
| 長検出器（He3_ELIGANT_Long） | 40本 |

---

## 環境構築

### 1. NBoxのビルド

```bash
cd <NBOX_SOURCE_DIR>
mkdir -p build && cd build
cmake ..
make -j4
```

### 2. Python環境の構築

```bash
cd <NBOX_SOURCE_DIR>/docs/optimization/phase2_bayesian
python3 -m venv venv
source venv/bin/activate
pip install optuna matplotlib numpy tqdm deap
```

### 3. 必要ファイルの確認

```bash
# 実行ファイル
ls <NBOX_BUILD_DIR>/nbox_sim

# 検出器定義ファイル
ls <NBOX_BUILD_DIR>/eligant_tn_detector.json

# ROOTの確認
root --version
# または
/opt/ROOT/bin/root --version
```

### 4. 環境変数の設定（オプション）

```bash
export NBOX_SOURCE_DIR=/path/to/NBox
export NBOX_BUILD_DIR=/path/to/NBox/build
```

---

## Phase 1: フラックスマップ解析

中性子フラックスの空間分布を解析し、検出器配置の最適領域を特定する。

### 1.1 シミュレーション実行

```bash
cd <NBOX_BUILD_DIR>

# フラックスマップ記録を有効化して実行
./nbox_sim -f \
    -g geometry.json \
    -d eligant_tn_detector.json \
    -m ../docs/optimization/phase1_fluxmap/configs/run_1MeV.mac
```

**オプション:**
- `-f`: フラックスマップ記録を有効化
- `-g`: ジオメトリファイル
- `-d`: 検出器定義ファイル
- `-m`: マクロファイル（エネルギー設定）

### 1.2 解析実行

```bash
cd <NBOX_BUILD_DIR>

# ROOTマクロで解析（TChainで直接読み込み、haddは不要）
root -l -b -q \
    '../docs/optimization/phase1_fluxmap/scripts/analyze_flux.C("output_run0_t*.root", "../docs/optimization/phase1_fluxmap/results", "1MeV", 10.0)'
```

**引数:**
1. 入力ファイルパターン（ワイルドカード使用可）
2. 出力ディレクトリ
3. エネルギーラベル
4. 最大半径 [cm]

### 1.3 出力ファイル

| ファイル | 説明 |
|---------|------|
| `flux_map_2d_*.png` | 2D フラックスマップ |
| `flux_radial_*.png` | 半径方向分布 |
| `flux_radial_*.csv` | 半径方向分布データ |

---

## Phase 2: ベイズ最適化

Optunaを使用したTPE（Tree-structured Parzen Estimator）による最適化。

### 2.1 制約なし最適化

検出器本数も最適化パラメータとして扱う。

```bash
cd <NBOX_SOURCE_DIR>/docs/optimization/phase2_bayesian
source venv/bin/activate

python scripts/bayesian_optimizer.py \
    --build-dir <NBOX_BUILD_DIR> \
    --detector-config <NBOX_BUILD_DIR>/eligant_tn_detector.json \
    --n-trials 50 \
    --n-events 10000 \
    --energy 1.0 \
    --energy-unit MeV \
    --n-rings 4 \
    --output results_unconstrained_1MeV
```

### 2.2 制約付き最適化（推奨）

全検出器を使用し、各リングは同一タイプの検出器のみ使用。

```bash
cd <NBOX_SOURCE_DIR>/docs/optimization/phase2_bayesian
source venv/bin/activate

python scripts/bayesian_optimizer_constrained.py \
    --build-dir <NBOX_BUILD_DIR> \
    --detector-config <NBOX_BUILD_DIR>/eligant_tn_detector.json \
    --n-trials 50 \
    --n-events 10000 \
    --energy 1.0 \
    --energy-unit MeV \
    --n-rings 4 \
    --n-short 28 \
    --n-long 40 \
    --output results_constrained_1MeV
```

### 2.3 オプション一覧

| オプション | 説明 | デフォルト |
|-----------|------|-----------|
| `--build-dir` | NBoxビルドディレクトリ | (必須) |
| `--detector-config` | 検出器定義JSON | (必須) |
| `--n-trials` | 最適化試行回数 | 50 |
| `--n-events` | シミュレーションイベント数 | 10000 |
| `--energy` | 中性子エネルギー | 1.0 |
| `--energy-unit` | エネルギー単位 | MeV |
| `--source-file` | スペクトルファイル（指定時はenergyを無視） | None |
| `--n-rings` | リング数（2, 3, 4） | 4 |
| `--n-short` | 短検出器本数 | 28 |
| `--n-long` | 長検出器本数 | 40 |
| `--short-type` | 短検出器タイプ名 | He3_ELIGANT |
| `--long-type` | 長検出器タイプ名 | He3_ELIGANT_Long |
| `--output` | 出力ディレクトリ | results |

### 2.4 Cf-252スペクトルでの最適化

```bash
python scripts/bayesian_optimizer_constrained.py \
    --build-dir <NBOX_BUILD_DIR> \
    --detector-config <NBOX_BUILD_DIR>/eligant_tn_detector.json \
    --source-file <NBOX_BUILD_DIR>/cf252_source.root \
    --n-trials 50 \
    --n-events 10000 \
    --output results_cf252
```

### 2.5 出力ファイル

| ファイル | 説明 |
|---------|------|
| `best_parameters.json` | 最適パラメータと効率 |
| `best_geometry.json` | 最適配置のジオメトリ（NBoxで直接使用可能） |
| `optimization_history.json` | 全試行の履歴 |
| `configs/trial_XXXX.json` | 各試行のジオメトリ設定 |

---

## Phase 3: 遺伝的アルゴリズム

DEAPライブラリを使用した進化的最適化。

### 3.1 最適化実行

```bash
cd <NBOX_SOURCE_DIR>/docs/optimization/phase2_bayesian
source venv/bin/activate

python ../phase3_genetic/genetic_optimizer.py \
    --build-dir <NBOX_BUILD_DIR> \
    --detector-config <NBOX_BUILD_DIR>/eligant_tn_detector.json \
    --n-generations 20 \
    --population 20 \
    --n-events 10000 \
    --energy 1.0 \
    --energy-unit MeV \
    --n-rings 4 \
    --n-short 28 \
    --n-long 40 \
    --output ../phase3_genetic/results_1MeV
```

### 3.2 オプション一覧

| オプション | 説明 | デフォルト |
|-----------|------|-----------|
| `--build-dir` | NBoxビルドディレクトリ | (必須) |
| `--detector-config` | 検出器定義JSON | (必須) |
| `--n-generations` | 世代数 | 20 |
| `--population` | 個体数 | 20 |
| `--n-events` | シミュレーションイベント数 | 10000 |
| `--energy` | 中性子エネルギー | 1.0 |
| `--energy-unit` | エネルギー単位 | MeV |
| `--source-file` | スペクトルファイル | None |
| `--n-rings` | リング数（2, 3, 4） | 4 |
| `--n-short` | 短検出器本数 | 28 |
| `--n-long` | 長検出器本数 | 40 |
| `--output` | 出力ディレクトリ | results_genetic |

### 3.3 アルゴリズム詳細

| 項目 | 設定 |
|------|------|
| 選択 | トーナメント選択 (tournsize=3) |
| 交叉確率 | 70% |
| 突然変異確率 | 30% |
| 制約 | 全検出器使用、リング内同一タイプ |

### 3.4 出力ファイル

| ファイル | 説明 |
|---------|------|
| `best_parameters.json` | 最適パラメータ |
| `best_geometry.json` | 最適配置のジオメトリ |
| `optimization_history.json` | 全評価の履歴 |
| `generation_stats.json` | 世代ごとの統計 |

---

## 結果の比較

### 1 MeV 制約付き最適化結果

| 手法 | 効率 | 評価回数 | 半径 [mm] | パターン | 本数 |
|------|------|---------|-----------|----------|------|
| **ベイズ** | **81.99%** | 50 | [42, 79, 124, 170] | S-L-S-L | [7, 15, 21, 25] |
| 遺伝的 | 80.04% | 185 | [50, 100, 144, 200] | S-L-S-L | [10, 20, 18, 20] |

### 10 MeV 制約付き最適化結果

| 手法 | 効率 | 半径 [mm] | パターン | 本数 |
|------|------|-----------|----------|------|
| ベイズ | 37.18% | [75, 142, 260, 347] | L-L-S-S | [14, 26, 12, 16] |

### 推奨事項

1. **手法**: ベイズ最適化（TPE）が効率的で高精度
2. **配置パターン**: 1 MeVでは「short-long-short-long」の交互配置が最適
3. **エネルギー依存性**: 高エネルギーでは最適配置が異なる

---

## 最適配置の使用方法

### nbox_simでの実行

```bash
cd <NBOX_BUILD_DIR>

./nbox_sim \
    -g /path/to/results/best_geometry.json \
    -d eligant_tn_detector.json \
    -m run.mac
```

### ジオメトリファイルの形式

```json
{
  "Box": {
    "Type": "Box",
    "x": 1000,
    "y": 1000,
    "z": 1000,
    "BeamPipe": 44
  },
  "Placements": [
    {
      "name": "A1",
      "type": "He3_ELIGANT",
      "R": 42.22,
      "Phi": 0.0
    },
    ...
  ]
}
```

---

## トラブルシューティング

### 1. ROOTが見つからない

```bash
# ROOTのパスを確認
which root
# または
/opt/ROOT/bin/root --version
```

スクリプト内のROOTパスを修正:
```python
# run_nbox.py 内
ROOT_PATH = "/opt/ROOT/bin/root"  # 環境に合わせて変更
```

### 2. シミュレーションがタイムアウト

`run_nbox.py` のタイムアウト値を増加:
```python
timeout = 300  # 秒
```

### 3. 無効な配置が多い

検出器本数の範囲を調整するか、リング数を減らす:
```bash
--n-rings 3  # 4から3に減らす
```

### 4. メモリ不足

イベント数を減らす:
```bash
--n-events 5000  # 10000から5000に減らす
```

### 5. 出力ファイルが見つからない

TChainで直接スレッドファイルを読み込むため、haddは不要:
```bash
# 正しい方法（ワイルドカード使用）
ls output_run0_t*.root
```

---

## ディレクトリ構成

```
docs/optimization/
├── README.md                    # このファイル
├── phase1_fluxmap/
│   ├── README.md
│   ├── configs/                 # マクロファイル
│   │   ├── run_1keV.mac
│   │   ├── run_1MeV.mac
│   │   └── ...
│   ├── scripts/
│   │   ├── analyze_flux.C       # ROOTマクロ
│   │   └── analyze_flux_root.py # Python版
│   └── results/                 # 解析結果
├── phase2_bayesian/
│   ├── README.md
│   ├── venv/                    # Python仮想環境
│   ├── scripts/
│   │   ├── bayesian_optimizer.py            # 制約なし版
│   │   ├── bayesian_optimizer_constrained.py # 制約付き版
│   │   ├── geometry_generator.py            # ジオメトリ生成
│   │   └── run_nbox.py                      # NBox実行ラッパー
│   └── results_*/               # 結果ディレクトリ
└── phase3_genetic/
    ├── README.md
    ├── genetic_optimizer.py     # 遺伝的アルゴリズム
    └── results_*/               # 結果ディレクトリ
```

---

## クイックスタート

### 最短手順（制約付きベイズ最適化）

```bash
# 1. 環境構築
cd <NBOX_SOURCE_DIR>/docs/optimization/phase2_bayesian
python3 -m venv venv
source venv/bin/activate
pip install optuna numpy tqdm

# 2. 最適化実行（1 MeV）
python scripts/bayesian_optimizer_constrained.py \
    --build-dir <NBOX_BUILD_DIR> \
    --detector-config <NBOX_BUILD_DIR>/eligant_tn_detector.json \
    --n-trials 50 \
    --n-events 10000 \
    --energy 1.0 \
    --output results_1MeV

# 3. 結果確認
cat results_1MeV/best_parameters.json

# 4. 最適配置でnbox_sim実行
cd <NBOX_BUILD_DIR>
./nbox_sim -g <NBOX_SOURCE_DIR>/docs/optimization/phase2_bayesian/results_1MeV/best_geometry.json \
           -d eligant_tn_detector.json \
           -m run.mac
```

### パスの置き換え

上記コマンド内の `<NBOX_SOURCE_DIR>` と `<NBOX_BUILD_DIR>` は実際のパスに置き換えてください:

```bash
# 例
NBOX_SOURCE_DIR=/home/user/NBox
NBOX_BUILD_DIR=/home/user/NBox/build
```
