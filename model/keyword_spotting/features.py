from __future__ import annotations

import torch
import torchaudio


class MFCCExtractor:
    """Extract fixed-size MFCC features matching the 16 kHz, 1 s setup."""

    def __init__(
        self,
        sample_rate: int = 16_000,
        num_mfcc: int = 13,
        num_mels: int = 26,
        n_fft: int = 512,
        win_length: int = 400,
        hop_length: int = 160,
    ) -> None:
        self.sample_rate = sample_rate
        self.n_fft = n_fft
        self.win_length = win_length
        self.hop_length = hop_length
        self.mfcc = torchaudio.transforms.MFCC(
            sample_rate=sample_rate,
            n_mfcc=num_mfcc,
            melkwargs={
                "n_fft": n_fft,
                "win_length": win_length,
                "hop_length": hop_length,
                "n_mels": num_mels,
                "center": False,
                "power": 2.0,
            },
            log_mels=True,
        )

    def __call__(self, waveform: torch.Tensor) -> torch.Tensor:
        """Return a normalized tensor with shape ``[1, 13, 99]``."""
        if waveform.ndim == 1:
            waveform = waveform.unsqueeze(0)
        # torchaudio frames against n_fft (not win_length): 512 + 98*160
        # samples are required for the 99-frame, 25 ms/10 ms setup.
        target = self.sample_rate + (self.n_fft - self.win_length) + self.hop_length
        if waveform.shape[-1] < target:
            waveform = torch.nn.functional.pad(waveform, (0, target - waveform.shape[-1]))
        else:
            waveform = waveform[..., :target]
        feat = self.mfcc(waveform).mean(dim=0)
        feat = (feat - feat.mean()) / (feat.std(unbiased=False) + 1e-6)
        return feat.unsqueeze(0)
