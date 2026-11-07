#!/usr/bin/env python
from __future__ import annotations

import argparse
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import torch

from keyword_spotting.data import KeywordDataset, AudioItem
from keyword_spotting.model import KeywordCNN


def main() -> None:
    p = argparse.ArgumentParser(description="Run one-file keyword inference")
    p.add_argument("audio")
    p.add_argument("--checkpoint", default="checkpoints/best.pt")
    p.add_argument("--threshold", type=float, default=None)
    p.add_argument("--device", default="cpu")
    args = p.parse_args()
    ckpt = torch.load(args.checkpoint, map_location=args.device, weights_only=False)
    model = KeywordCNN().to(args.device)
    model.load_state_dict(ckpt["model"])
    model.eval()
    threshold = ckpt.get("threshold", 0.5) if args.threshold is None else args.threshold
    x, _ = KeywordDataset([AudioItem(args.audio, 0)])[0]
    with torch.no_grad():
        probability = torch.sigmoid(model(x.unsqueeze(0).to(args.device))).item()
    print(f"probability={probability:.6f}")
    print(f"prediction={'positive (Hey Acho)' if probability >= threshold else 'negative'}")


if __name__ == "__main__":
    main()
