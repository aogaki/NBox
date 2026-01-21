# He-3検出器配置最適化 詳細実装計画書

## 1. 概要

本文書は「01_specification.md」で定義された仕様に基づき、各Phaseの詳細な実装計画を記述する。

---

## 2. 前提作業: NBox機能拡張

### 2.1 モノエナジー中性子源の設定

#### 現状
Geant4標準のマクロコマンドでモノエナジー中性子を設定可能：
```
/gun/particle neutron
/gun/energy 1 keV
```

#### 実装内容
各エネルギー用のマクロファイルを作成：

**run_1keV.mac**
```
/gun/particle neutron
/gun/energy 1 keV
/run/beamOn 100000
```

同様に `run_10keV.mac`, `run_100keV.mac`, `run_1MeV.mac`, `run_5MeV.mac`, `run_10MeV.mac` を作成。

#### 検証
- 各エネルギーで中性子を生成し、エネルギー分布を確認

---

### 2.2 シミュレーション実行・結果マージスクリプト

#### 目的
シミュレーション実行とROOTファイルのマージを自動化する。

#### 実装内容

**run_simulation.sh**
```bash
#!/bin/bash
# Usage: ./run_simulation.sh <energy> <unit> <nevents>
# Example: ./run_simulation.sh 1 keV 100000

ENERGY=$1
UNIT=$2
NEVENTS=$3
OUTPUT_NAME="${ENERGY}${UNIT}"

# Run NBox
./NBox -g geometry.json -d detector.json -m run.mac -n $NEVENTS

# Merge thread files
hadd -f ${OUTPUT_NAME}.root NBox_t*.root

# Cleanup thread files (optional)
rm -f NBox_t*.root
```

**run_all_energies.sh**
```bash
#!/bin/bash
# Run simulations for all energy points

NEVENTS=100000

./run_simulation.sh 1 keV $NEVENTS
./run_simulation.sh 10 keV $NEVENTS
./run_simulation.sh 100 keV $NEVENTS
./run_simulation.sh 1 MeV $NEVENTS
./run_simulation.sh 5 MeV $NEVENTS
./run_simulation.sh 10 MeV $NEVENTS
```

---

## 3. Phase 1: フラックスマップ実装

### 3.1 概要

モデレータ内の熱中性子フラックス分布を計測するため、ボクセル化したモデレータ内で中性子のステップを記録する。

### 3.2 実装方針

#### 方針A: SteppingAction方式（推奨）
- 全ステップで中性子の位置・エネルギーを記録
- 熱中性子（E < 0.5 eV）のみカウント
- ボクセルへのビニングは解析時に実施

**利点**: ジオメトリ変更不要、柔軟な解析が可能
**欠点**: データ量が大きくなる可能性

#### 方針B: ボクセルSensitiveDetector方式
- モデレータをボクセルに分割し、各ボクセルをSensitiveDetectorとして登録
- 各ボクセルでの熱中性子通過をカウント

**利点**: リアルタイムでビニング、データ量削減
**欠点**: ジオメトリ構築が複雑

### 3.3 方針A（SteppingAction方式）の詳細設計

#### 3.3.1 データ構造

記録する情報（各ステップ）:
- 位置 (x, y, z)
- 中性子エネルギー
- ステップ長
- イベント番号

#### 3.3.2 実装ファイル

**FluxSteppingAction.hh / .cc**
```cpp
class FluxSteppingAction : public G4UserSteppingAction
{
public:
    void UserSteppingAction(const G4Step* step) override;

private:
    static constexpr G4double kThermalEnergyCut = 0.5 * eV;
};
```

**FluxRunAction.hh / .cc**
- ROOTファイルへの出力管理
- TTree構造:
  ```
  TTree "flux"
  ├── eventID (Int_t)
  ├── x, y, z (Double_t) [mm]
  ├── energy (Double_t) [eV]
  └── stepLength (Double_t) [mm]
  ```

#### 3.3.3 ビルド設定

フラックスマップモードを有効にするコンパイルオプション:
```cmake
option(ENABLE_FLUXMAP "Enable flux map recording" OFF)
if(ENABLE_FLUXMAP)
    add_definitions(-DFLUXMAP_MODE)
endif()
```

または実行時オプション:
```
./NBox --fluxmap -g geometry.json ...
```

### 3.4 解析スクリプト

#### 3.4.1 analyze_flux.py

```python
"""
フラックスマップ解析スクリプト

入力: シミュレーション結果のROOTファイル
出力: 3Dヒストグラム、2D断面図、半径方向プロファイル
"""

import uproot
import numpy as np
import matplotlib.pyplot as plt

class FluxMapAnalyzer:
    def __init__(self, root_file, voxel_size=5.0):
        """
        Parameters:
        -----------
        root_file : str
            入力ROOTファイルパス
        voxel_size : float
            ボクセルサイズ [mm]
        """
        self.voxel_size = voxel_size
        self.data = self._load_data(root_file)

    def _load_data(self, root_file):
        """ROOTファイルからデータを読み込み"""
        pass

    def create_3d_histogram(self):
        """3Dフラックスマップを生成"""
        pass

    def plot_xy_slice(self, z=0):
        """XY断面図を描画"""
        pass

    def plot_xz_slice(self, y=0):
        """XZ断面図を描画"""
        pass

    def plot_radial_profile(self):
        """半径方向フラックスプロファイルを描画"""
        pass

    def find_optimal_radius(self):
        """最適な検出器配置半径を推定"""
        pass
```

#### 3.4.2 plot_fluxmap.py

可視化専用スクリプト:
- matplotlib/plotlyによる2D/3Dプロット
- エネルギー比較プロット
- アニメーション生成（オプション）

### 3.5 実行手順

```bash
# 1. フラックスマップ用ジオメトリでビルド
cd build
cmake .. -DENABLE_FLUXMAP=ON
make -j4

# 2. 各エネルギーでシミュレーション実行
cd ../docs/optimization/phase1_fluxmap
./scripts/run_all_energies.sh

# 3. 解析実行
python scripts/analyze_flux.py simulations/1keV.root -o results/

# 4. 比較プロット生成
python scripts/plot_fluxmap.py --compare simulations/*.root -o results/
```

### 3.6 期待される出力

- `results/fluxmap_1keV_xy.png` - XY断面図（z=0）
- `results/fluxmap_1keV_xz.png` - XZ断面図（y=0）
- `results/radial_profile_1keV.png` - 半径方向プロファイル
- `results/radial_profile_comparison.png` - 全エネルギー比較
- `results/optimal_radii.csv` - 各エネルギーでの推定最適半径

---

## 4. Phase 2: ベイズ最適化実装

### 4.1 概要

Gaussian Processを用いて検出効率を予測し、少ない試行で最適な検出器配置を探索する。

### 4.2 ツール選定

**Optuna**（推奨）
- 使いやすいAPI
- 複数のサンプラー対応（TPE, GP, CMA-ES）
- 可視化機能内蔵
- 並列実行サポート

### 4.3 実装設計

#### 4.3.1 最適化パラメータ

| パラメータ | 型 | 範囲 | 説明 |
|-----------|-----|------|------|
| r1 | float | [35, 80] | リングA半径 [mm] |
| r2 | float | [80, 130] | リングB半径 [mm] |
| r3 | float | [130, 180] | リングC半径 [mm] |
| r4 | float | [180, 220] | リングD半径 [mm] |
| n1 | int | [4, 12] | リングA本数 |
| n2 | int | [8, 20] | リングB本数 |
| n3 | int | [12, 24] | リングC本数 |
| n4 | int | [16, 28] | リングD本数 |

#### 4.3.2 制約条件

```python
def is_valid_configuration(r1, r2, r3, r4, n1, n2, n3, n4):
    """配置の物理的妥当性をチェック"""

    DETECTOR_DIAMETER = 25.4  # mm
    MIN_GAP = 5.0  # mm
    BEAM_PIPE_RADIUS = 22.0  # mm
    BOX_HALF_WIDTH = 230.0  # mm

    # リング間の最小距離
    if r2 - r1 < DETECTOR_DIAMETER + MIN_GAP:
        return False
    if r3 - r2 < DETECTOR_DIAMETER + MIN_GAP:
        return False
    if r4 - r3 < DETECTOR_DIAMETER + MIN_GAP:
        return False

    # ビームパイプとの干渉
    if r1 - DETECTOR_DIAMETER/2 < BEAM_PIPE_RADIUS:
        return False

    # モデレータ壁との干渉
    if r4 + DETECTOR_DIAMETER/2 > BOX_HALF_WIDTH:
        return False

    # 同一リング内の検出器間隔
    for r, n in [(r1, n1), (r2, n2), (r3, n3), (r4, n4)]:
        circumference = 2 * np.pi * r
        spacing = circumference / n
        if spacing < DETECTOR_DIAMETER + MIN_GAP:
            return False

    return True
```

#### 4.3.3 目的関数

```python
def objective(trial):
    """最適化の目的関数"""

    # パラメータサンプリング
    r1 = trial.suggest_float('r1', 35, 80)
    r2 = trial.suggest_float('r2', 80, 130)
    r3 = trial.suggest_float('r3', 130, 180)
    r4 = trial.suggest_float('r4', 180, 220)
    n1 = trial.suggest_int('n1', 4, 12)
    n2 = trial.suggest_int('n2', 8, 20)
    n3 = trial.suggest_int('n3', 12, 24)
    n4 = trial.suggest_int('n4', 16, 28)

    # 制約チェック
    if not is_valid_configuration(r1, r2, r3, r4, n1, n2, n3, n4):
        return 0.0  # 無効な配置は効率0

    # ジオメトリファイル生成
    config = generate_geometry_json(r1, r2, r3, r4, n1, n2, n3, n4)
    config_path = f"configs/trial_{trial.number}.json"
    save_json(config, config_path)

    # シミュレーション実行
    result_path = f"simulations/trial_{trial.number}.root"
    run_nbox(config_path, result_path, nevents=10000)

    # 効率計算
    efficiency = calculate_efficiency(result_path)

    return efficiency
```

### 4.4 ファイル構成

```
phase2_bayesian/
├── scripts/
│   ├── bayesian_optimizer.py    # メイン最適化スクリプト
│   ├── run_nbox.py              # NBox実行ラッパー
│   ├── geometry_generator.py    # JSONジオメトリ生成
│   ├── efficiency_calculator.py # 効率計算
│   └── visualize_results.py     # 結果可視化
├── configs/
│   └── optimization_config.yaml # 最適化設定
├── simulations/
│   └── (trial_*.root)
└── results/
    ├── optimization_history.csv
    ├── best_parameters.json
    └── convergence_plot.png
```

### 4.5 実行手順

```bash
cd docs/optimization/phase2_bayesian

# 最適化実行（例: 1MeV中性子、100回試行）
python scripts/bayesian_optimizer.py \
    --energy 1MeV \
    --n-trials 100 \
    --n-events 10000 \
    --output results/

# 結果可視化
python scripts/visualize_results.py results/optimization_history.csv
```

---

## 5. Phase 3: 遺伝的アルゴリズム実装

### 5.1 概要

検出器配置を遺伝子として表現し、進化的に最適化する。

### 5.2 ツール選定

**DEAP**（推奨）
- 柔軟なGA実装
- カスタム演算子定義が容易
- マルチプロセス対応

### 5.3 実装設計

#### 5.3.1 遺伝子表現

```python
# 個体 = [r1, r2, r3, r4, n1, n2, n3, n4]
# 例: [59.0, 110.0, 155.0, 195.0, 8, 16, 20, 28]

Individual = list  # 8要素のリスト
```

#### 5.3.2 遺伝的演算子

**選択**: トーナメント選択（サイズ3）
**交叉**: ブレンド交叉（BLX-α, α=0.5）
**突然変異**: ガウス突然変異（σ=5.0 for radius, σ=1 for count）

```python
from deap import base, creator, tools, algorithms

# 適応度（最大化）
creator.create("FitnessMax", base.Fitness, weights=(1.0,))
creator.create("Individual", list, fitness=creator.FitnessMax)

toolbox = base.Toolbox()

# 個体生成
toolbox.register("attr_r1", random.uniform, 35, 80)
toolbox.register("attr_r2", random.uniform, 80, 130)
toolbox.register("attr_r3", random.uniform, 130, 180)
toolbox.register("attr_r4", random.uniform, 180, 220)
toolbox.register("attr_n1", random.randint, 4, 12)
toolbox.register("attr_n2", random.randint, 8, 20)
toolbox.register("attr_n3", random.randint, 12, 24)
toolbox.register("attr_n4", random.randint, 16, 28)

# 演算子登録
toolbox.register("evaluate", evaluate_individual)
toolbox.register("mate", tools.cxBlend, alpha=0.5)
toolbox.register("mutate", custom_gaussian_mutation)
toolbox.register("select", tools.selTournament, tournsize=3)
```

#### 5.3.3 GAパラメータ

| パラメータ | 値 | 説明 |
|-----------|-----|------|
| 集団サイズ | 50 | 各世代の個体数 |
| 世代数 | 50 | 最大世代数 |
| 交叉率 | 0.7 | 交叉を適用する確率 |
| 突然変異率 | 0.2 | 突然変異を適用する確率 |
| エリート数 | 5 | 次世代に直接引き継ぐ個体数 |

### 5.4 ファイル構成

```
phase3_genetic/
├── scripts/
│   ├── genetic_optimizer.py     # メインGAスクリプト
│   ├── run_nbox.py              # NBox実行ラッパー（phase2と共有可）
│   ├── operators.py             # カスタム遺伝的演算子
│   └── visualize_evolution.py   # 進化履歴可視化
├── configs/
│   └── ga_config.yaml           # GA設定
├── simulations/
│   └── (gen_*_ind_*.root)
└── results/
    ├── evolution_history.csv
    ├── best_individuals.json
    ├── fitness_plot.png
    └── pareto_front.png          # 多目的の場合
```

### 5.5 実行手順

```bash
cd docs/optimization/phase3_genetic

# GA実行
python scripts/genetic_optimizer.py \
    --energy 1MeV \
    --population 50 \
    --generations 50 \
    --n-events 10000 \
    --output results/

# 進化履歴可視化
python scripts/visualize_evolution.py results/evolution_history.csv
```

---

## 6. Phase 4: 比較・検証

### 6.1 比較項目

1. **最適効率の比較**
   - 各手法で得られた最高効率
   - フラックスマップ予測との差異

2. **計算コストの比較**
   - シミュレーション実行回数
   - 総計算時間

3. **解の質**
   - 解の安定性（複数回実行での変動）
   - 局所解への陥りやすさ

4. **エネルギー依存性**
   - 各エネルギーでの最適配置の違い
   - 汎用的な配置の可能性

### 6.2 比較スクリプト

```python
"""compare_methods.py - 手法間比較"""

class MethodComparator:
    def __init__(self):
        self.results = {}

    def load_fluxmap_results(self, path):
        """Phase 1の結果を読み込み"""
        pass

    def load_bayesian_results(self, path):
        """Phase 2の結果を読み込み"""
        pass

    def load_genetic_results(self, path):
        """Phase 3の結果を読み込み"""
        pass

    def compare_efficiency(self):
        """効率比較テーブルを生成"""
        pass

    def compare_computational_cost(self):
        """計算コスト比較"""
        pass

    def generate_report(self, output_path):
        """比較レポートを生成"""
        pass
```

### 6.3 出力

- `comparison_report.md` - 比較結果のMarkdownレポート
- `efficiency_comparison.png` - 効率比較グラフ
- `cost_comparison.png` - 計算コスト比較グラフ
- `optimal_configurations.json` - 各手法の最適配置

---

## 7. 共通ユーティリティ

### 7.1 NBox実行ラッパー (run_nbox.py)

```python
"""NBoxシミュレーション実行ユーティリティ"""

import subprocess
import os
from pathlib import Path

class NBoxRunner:
    def __init__(self, nbox_path, build_dir):
        self.nbox_path = nbox_path
        self.build_dir = build_dir

    def run(self, geometry_file, detector_file,
            energy, energy_unit, nevents, output_dir):
        """
        NBoxシミュレーションを実行

        Parameters:
        -----------
        geometry_file : str
            ジオメトリ設定ファイル
        detector_file : str
            検出器設定ファイル
        energy : float
            中性子エネルギー
        energy_unit : str
            エネルギー単位 (keV, MeV)
        nevents : int
            イベント数
        output_dir : str
            出力ディレクトリ
        """
        # マクロファイル生成
        macro_content = f"""
/gun/particle neutron
/gun/energy {energy} {energy_unit}
/run/beamOn {nevents}
"""
        macro_path = Path(output_dir) / "run.mac"
        macro_path.write_text(macro_content)

        # 実行
        cmd = [
            self.nbox_path,
            "-g", geometry_file,
            "-d", detector_file,
            "-m", str(macro_path)
        ]

        subprocess.run(cmd, cwd=self.build_dir, check=True)

        # 結果マージ
        self._merge_results(output_dir, energy, energy_unit)

    def _merge_results(self, output_dir, energy, energy_unit):
        """スレッドファイルをマージ"""
        output_name = f"{energy}{energy_unit}.root"
        thread_files = list(Path(self.build_dir).glob("NBox_t*.root"))

        if thread_files:
            cmd = ["hadd", "-f", str(Path(output_dir) / output_name)]
            cmd.extend([str(f) for f in thread_files])
            subprocess.run(cmd, check=True)

            # クリーンアップ
            for f in thread_files:
                f.unlink()
```

### 7.2 効率計算

既存の `build/analyze_efficiency.C` を使用する。

#### 使用方法
```bash
cd build
root -l -b -q analyze_efficiency.C
```

#### 機能
- スレッドファイル（`output_run0_t*.root`）を自動マージ
- 全体効率・リング別効率・検出器別効率を計算
- エネルギー閾値 vs 効率のグラフ作成
- プロット出力（`efficiency_analysis.png`, `per_detector_analysis.png`）

#### 最適化での活用
AI最適化スクリプトから呼び出す場合のラッパー:

```python
"""efficiency_wrapper.py - analyze_efficiency.Cのラッパー"""

import subprocess
import re
from pathlib import Path

def run_efficiency_analysis(work_dir, n_neutrons=100000):
    """
    analyze_efficiency.Cを実行して効率を取得

    Parameters:
    -----------
    work_dir : str
        ROOTファイルがあるディレクトリ
    n_neutrons : int
        入射中性子数（analyze_efficiency.C内の値と一致させる）

    Returns:
    --------
    dict
        効率情報 {'intrinsic': float, 'full_energy': float, 'per_ring': dict}
    """
    # analyze_efficiency.Cを実行
    result = subprocess.run(
        ['root', '-l', '-b', '-q', 'analyze_efficiency.C'],
        cwd=work_dir,
        capture_output=True,
        text=True
    )

    # 出力から効率を抽出
    output = result.stdout
    efficiency = {}

    # Intrinsic efficiency
    match = re.search(r'Intrinsic efficiency.*: ([\d.]+) %', output)
    if match:
        efficiency['intrinsic'] = float(match.group(1))

    # Full-energy efficiency
    match = re.search(r'Full-energy efficiency.*: ([\d.]+) %', output)
    if match:
        efficiency['full_energy'] = float(match.group(1))

    return efficiency
```

---

## 8. 実装順序

### Step 1: 基盤整備
1. [ ] モノエナジー中性子源のマクロファイル作成（各エネルギー用）
2. [ ] シミュレーション実行・結果マージスクリプト作成
3. [ ] 効率計算ラッパー作成（既存 `analyze_efficiency.C` を活用）

### Step 2: Phase 1 実装
4. [ ] フラックス記録用SteppingAction実装
5. [ ] フラックスマップ解析スクリプト作成
6. [ ] 各エネルギーでシミュレーション実行
7. [ ] 結果解析・可視化

### Step 3: Phase 2 実装
8. [ ] ベイズ最適化スクリプト作成
9. [ ] 制約条件・目的関数実装
10. [ ] 各エネルギーで最適化実行
11. [ ] 結果解析

### Step 4: Phase 3 実装
12. [ ] 遺伝的アルゴリズムスクリプト作成
13. [ ] 遺伝的演算子実装
14. [ ] 各エネルギーで最適化実行
15. [ ] 結果解析

### Step 5: Phase 4 実装
16. [ ] 比較スクリプト作成
17. [ ] 最終レポート作成

---

## 改訂履歴

| 日付 | バージョン | 変更内容 |
|------|-----------|----------|
| 2026-01-21 | 0.1 | 初版作成 |
