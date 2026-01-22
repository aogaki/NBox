# Phase 1: フラックスマップ

## 概要

モデレータ内の熱中性子フラックス分布を直接計測し、最適な検出器配置を導出する。

## 使用方法

### 1. シミュレーション実行

```bash
# buildディレクトリから実行

# 単一エネルギー
./NBox -f -g 4rings.json -d eligant_tn_detector.json -m ../docs/optimization/phase1_fluxmap/configs/run_1MeV.mac

# 全エネルギーを連続実行
cd ../docs/optimization/phase1_fluxmap/scripts
./run_all_energies.sh
```

**オプション説明:**
- `-f`: フラックスマップ記録を有効化（FluxMap Ntupleを出力）
- `-g`: ジオメトリファイル
- `-d`: 検出器定義ファイル
- `-m`: マクロファイル（エネルギー設定）

### 2. 解析

解析スクリプト（`analyze_flux.C`）はTChainを使用してスレッドファイルを直接読み込むため、事前のマージ（hadd）は不要です。

```bash
cd /Users/aogaki/WorkSpace/NBox/build

# ROOTマクロで解析（TChainで直接スレッドファイルを読み込み）
root -l -b -q '../docs/optimization/phase1_fluxmap/scripts/analyze_flux.C("output_run0_t*.root", "../docs/optimization/phase1_fluxmap/results", "1MeV", 10.0)'
```

Pythonスクリプトを使う場合:

```bash
cd docs/optimization/phase1_fluxmap/scripts

# uproot + TChainで解析（マージ不要）
python analyze_flux_root.py /path/to/build/output_run0_t*.root -o ../results/ -v 10
```

## 出力ファイル

### シミュレーション出力 (ROOTファイル)

#### NBox Ntuple (既存)
検出イベント情報（He-3検出器での反応）

| カラム | 型 | 説明 |
|--------|-----|------|
| EventID | Int | イベント番号 |
| DetectorID | Int | 検出器ID |
| DetectorName | String | 検出器名 |
| Edep_keV | Double | エネルギー付与 [keV] |
| Time_ns | Double | 検出時刻 [ns] |

#### FluxMap Ntuple (新規、`-f`オプション時)
熱中性子（E < 0.5 eV）の位置情報

| カラム | 型 | 説明 |
|--------|-----|------|
| EventID | Int | イベント番号 |
| X_mm | Double | X座標 [mm] |
| Y_mm | Double | Y座標 [mm] |
| Z_mm | Double | Z座標 [mm] |
| Energy_eV | Double | 運動エネルギー [eV] |
| StepLength_mm | Double | ステップ長 [mm] |

### 解析出力

- `fluxmap_<energy>_xy.png` - XY断面図（Z=0）
- `fluxmap_<energy>_xz.png` - XZ断面図（Y=0）
- `radial_profile_<energy>.png` - 半径方向フラックスプロファイル
- `radial_profile_comparison.png` - 全エネルギー比較
- `optimal_radii.csv` - 各エネルギーでの最適半径

## 解析パラメータ

- **ボクセルサイズ**: 10 mm（デフォルト）
- **熱中性子閾値**: 0.5 eV

## 必要なPythonパッケージ

```bash
pip install uproot awkward numpy matplotlib
```

## ディレクトリ構成

```
phase1_fluxmap/
├── README.md           # 本ファイル
├── configs/            # マクロファイル
│   ├── run_1keV.mac
│   ├── run_10keV.mac
│   ├── run_100keV.mac
│   ├── run_1MeV.mac
│   ├── run_5MeV.mac
│   └── run_10MeV.mac
├── scripts/            # 解析スクリプト
│   ├── analyze_flux.C          # ROOTマクロ（TChain使用、マージ不要）
│   ├── analyze_flux.py         # Python版（uproot使用）
│   ├── analyze_flux_root.py    # Python版（ROOT/TChain使用）
│   └── compare_energies.py     # エネルギー比較
├── simulations/        # シミュレーション結果
│   └── (*.root)
└── results/            # 解析結果
    └── (*.png, *.csv)
```
