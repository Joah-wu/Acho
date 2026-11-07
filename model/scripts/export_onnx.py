#!/usr/bin/env python
"""Export a trained KeywordCNN checkpoint to an ONNX probability model."""
from __future__ import annotations

import argparse
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import torch
from torch import nn

from keyword_spotting.model import KeywordCNN


class ProbabilityModel(nn.Module):
    """Expose a probability instead of the training-time logit."""

    def __init__(self, model: nn.Module) -> None:
        super().__init__()
        self.model = model

    def forward(self, mfcc: torch.Tensor) -> torch.Tensor:
        return torch.sigmoid(self.model(mfcc)).unsqueeze(-1)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--checkpoint", default="checkpoints/best.pt")
    parser.add_argument("--output", default="checkpoints/keyword_cnn.onnx")
    parser.add_argument("--opset", type=int, default=17)
    parser.add_argument("--device", default="cpu", choices=["cpu", "cuda"])
    args = parser.parse_args()

    if args.device == "cuda" and not torch.cuda.is_available():
        raise RuntimeError("CUDA was requested but is not available")
    checkpoint = torch.load(args.checkpoint, map_location=args.device, weights_only=False)
    model = KeywordCNN()
    model.load_state_dict(checkpoint["model"])
    wrapper = ProbabilityModel(model).to(args.device).eval()
    dummy = torch.zeros(1, 1, 13, 99, device=args.device)
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)

    with torch.no_grad():
        torch.onnx.export(
            wrapper,
            dummy,
            str(output),
            input_names=["mfcc"],
            output_names=["probability"],
            dynamic_axes={"mfcc": {0: "batch"}, "probability": {0: "batch"}},
            opset_version=args.opset,
            do_constant_folding=True,
        )
    print(f"exported: {output}")
    print("input: mfcc [batch, 1, 13, 99] float32")
    print("output: probability [batch, 1] float32")
    print(f"threshold: {checkpoint.get('threshold', 0.5)}")


if __name__ == "__main__":
    main()
