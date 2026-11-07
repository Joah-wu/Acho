#!/usr/bin/env python
from __future__ import annotations

import argparse
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import torch
from torch.utils.data import DataLoader

from keyword_spotting.data import KeywordDataset, read_manifest, scan_audio, stratified_split
from keyword_spotting.model import KeywordCNN
from keyword_spotting.utils import binary_metrics


def main() -> None:
    p = argparse.ArgumentParser(description="Evaluate a trained keyword detector")
    p.add_argument("--checkpoint", default="checkpoints/best.pt")
    p.add_argument("--data-dir", default="data")
    p.add_argument("--manifest", default=None)
    p.add_argument("--threshold", type=float, default=None)
    p.add_argument("--batch-size", type=int, default=32)
    p.add_argument("--device", default="cpu")
    args = p.parse_args()
    ckpt = torch.load(args.checkpoint, map_location=args.device, weights_only=False)
    model = KeywordCNN()
    model.load_state_dict(ckpt["model"])
    model.to(args.device).eval()
    threshold = ckpt.get("threshold", 0.5) if args.threshold is None else args.threshold
    items = read_manifest(args.manifest) if args.manifest else stratified_split(scan_audio(args.data_dir), seed=42)[1]
    loader = DataLoader(KeywordDataset(items), batch_size=args.batch_size, shuffle=False)
    labels, predictions = [], []
    with torch.no_grad():
        for x, y in loader:
            probability = torch.sigmoid(model(x.to(args.device))).cpu()
            predictions.extend((probability >= threshold).int().tolist())
            labels.extend(y.int().tolist())
    metrics = binary_metrics(labels, predictions)
    print(f"threshold={threshold:.3f} samples={len(labels)}")
    for name in ("accuracy", "precision", "recall", "f1", "tp", "tn", "fp", "fn"):
        print(f"{name}={metrics[name]:.4f}" if isinstance(metrics[name], float) else f"{name}={metrics[name]}")


if __name__ == "__main__":
    main()
