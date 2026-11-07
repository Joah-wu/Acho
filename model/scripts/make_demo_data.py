#!/usr/bin/env python
"""Generate tiny synthetic data to smoke-test the complete pipeline."""
from __future__ import annotations

import argparse
import math
from pathlib import Path

import torch
import torchaudio


def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument("--output", default="data")
    p.add_argument("--per-class", type=int, default=20)
    p.add_argument("--sample-rate", type=int, default=16_000)
    args = p.parse_args()
    out = Path(args.output)
    rate, n = args.sample_rate, args.sample_rate
    t = torch.arange(n, dtype=torch.float32) / rate
    gen = torch.Generator().manual_seed(7)
    for label, folder in ((1, "positive"), (0, "negative")):
        (out / folder).mkdir(parents=True, exist_ok=True)
        for i in range(args.per_class):
            noise = 0.025 * torch.randn(n, generator=gen)
            if label:
                phase = torch.rand((), generator=gen).item() * math.tau
                signal = 0.25 * torch.sin(2 * math.pi * 520 * t + phase)
                signal += 0.20 * torch.sin(2 * math.pi * 760 * t + phase / 2)
            else:
                frequency = 120 + 700 * torch.rand((), generator=gen)
                signal = 0.05 * torch.sin(2 * math.pi * frequency * t)
            torchaudio.save(str(out / folder / f"demo_{i:04d}.wav"), (signal + noise).unsqueeze(0), rate)
    print(f"generated {args.per_class * 2} files under {out}")


if __name__ == "__main__":
    main()
