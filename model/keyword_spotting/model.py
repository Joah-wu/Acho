from __future__ import annotations

import torch
from torch import nn


class FixedAdaptivePool2d(nn.Module):
    """Equivalent to adaptive pooling for the fixed 11-column feature map.

    Explicit slices export cleanly to ONNX, unlike AdaptiveAvgPool2d((1, 4))
    in the legacy exporter used by PyTorch 2.5.
    """

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        parts = (x[..., 0:3], x[..., 3:6], x[..., 6:9], x[..., 9:11])
        return torch.cat([part.mean(dim=-1, keepdim=True) for part in parts], dim=-1)


class KeywordCNN(nn.Module):
    """Small CNN suitable for real-time keyword detection on a microcontroller."""

    def __init__(self) -> None:
        super().__init__()
        self.features = nn.Sequential(
            nn.Conv2d(1, 4, kernel_size=3, padding=1),
            nn.ReLU(inplace=True),
            nn.MaxPool2d(kernel_size=2, stride=3),
            nn.Conv2d(4, 8, kernel_size=3, padding=1),
            nn.ReLU(inplace=True),
            nn.MaxPool2d(kernel_size=2, stride=3),
            FixedAdaptivePool2d(),
        )
        self.classifier = nn.Sequential(
            nn.Flatten(),
            nn.Linear(8 * 4, 64),
            nn.ReLU(inplace=True),
            nn.Linear(64, 32),
            nn.ReLU(inplace=True),
            nn.Linear(32, 1),
        )

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        return self.classifier(self.features(x)).squeeze(-1)
