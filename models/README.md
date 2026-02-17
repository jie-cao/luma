# LUMA AI 模型文件

本目录用于存放 AI 面部重建所需的 ONNX 模型文件。

**模型文件必须放在此目录中**，即项目根目录下的 `models/` 文件夹：
```
c:\code\luma\models\
    det_10g.onnx        (面部检测，~16MB)
    2d106det.onnx       (面部关键点，~5MB)
    mb1_120x120.onnx    (3D面部重建，~5MB)
```

如果缺少模型文件，程序仍可运行，但"从照片生成"功能只能提取肤色，
无法进行精确的面部形状重建。

---

## 模型 1 & 2：InsightFace（面部检测 + 关键点）

### 直接下载（推荐）

从 HuggingFace 直接下载，无需注册：

**det_10g.onnx**（面部检测模型，16.9MB）：
```
https://huggingface.co/lithiumice/insightface/resolve/main/models/buffalo_l/det_10g.onnx?download=true
```

**2d106det.onnx**（106点面部关键点模型，5MB）：
```
https://huggingface.co/lithiumice/insightface/resolve/main/models/buffalo_l/2d106det.onnx?download=true
```

下载后将两个 `.onnx` 文件直接放入本目录（`models/`）。

### 备用方法：用 pip 下载

如果上面的链接失效，可以用 Python 安装 insightface 包然后提取：
```bash
pip install insightface
python -c "import insightface; insightface.app.FaceAnalysis(name='buffalo_l')"
```
模型会下载到 `~/.insightface/models/buffalo_l/`，从中复制
`det_10g.onnx` 和 `2d106det.onnx` 到本目录。

---

## 模型 3：3DDFA_V2（3D 面部重建）

3DDFA_V2 官方仓库只提供 PyTorch 格式（`.pth`），需要转换为 ONNX。

### 方法 A：用 Python 脚本自动转换（推荐）

1. 安装依赖：
```bash
pip install torch onnx
```

2. 运行以下 Python 脚本（已提供在 `models/convert_3ddfa.py`）：
```bash
cd models
python convert_3ddfa.py
```

脚本会自动下载 PyTorch 权重并转换为 `mb1_120x120.onnx`。

### 方法 B：手动操作

1. 克隆 3DDFA_V2 仓库：
   ```bash
   git clone https://github.com/cleardusk/3DDFA_V2.git
   ```
2. 进入仓库目录，运行 demo 时加 `--onnx` 参数，会自动生成 ONNX 文件：
   ```bash
   cd 3DDFA_V2
   python demo.py --onnx
   ```
3. 在仓库的 `weights/` 目录中找到生成的 `mb1_120x120.onnx`，
   复制到本目录（`models/`）。

---

## 验证

放好所有模型后，目录结构应该是：
```
models/
├── det_10g.onnx          ← InsightFace 面部检测
├── 2d106det.onnx         ← InsightFace 面部关键点
├── mb1_120x120.onnx      ← 3DDFA_V2 3D面部重建
├── convert_3ddfa.py      ← 转换脚本
└── README.md             ← 本文件
```

重新启动 LUMA Studio，"From Photo (AI)" 按钮应该从黄色（Basic）变为正常颜色，
控制台不再显示模型缺失警告。

---

## 无模型时的降级行为

| 功能 | 有模型 | 无模型 (Basic 模式) |
|------|--------|---------------------|
| 面部检测 | RetinaFace 高精度检测 | 肤色直方图粗略定位 |
| 关键点 | 106点精确定位 | 从边界框几何推算68点 |
| 3D重建 | 3DMM 系数回归 | 仅用宽高比估算，接近默认值 |
| 肤色提取 | 基于关键点精确采样 | 基于粗略区域采样（仍可用） |
