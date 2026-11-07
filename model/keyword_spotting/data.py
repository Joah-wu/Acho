from __future__ import annotations

import csv
import random
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

import torch
import torchaudio
from torch.utils.data import Dataset

from .features import MFCCExtractor


POSITIVE_NAMES = {"positive", "wake_word", "wakeword", "1", "hey_acho", "hey acho"}
NEGATIVE_NAMES = {"negative", "background", "noise", "unknown", "0"}


@dataclass(frozen=True)
class AudioItem:
    path: str
    label: int


def _label_from_parent(path: Path) -> int:
    name = path.parent.name.lower().strip()
    if name in POSITIVE_NAMES:
        return 1
    if name in NEGATIVE_NAMES:
        return 0
    raise ValueError(
        f"Cannot infer label from {path}. Put files under positive/ and negative/ directories."
    )


def scan_audio(data_dir: str | Path) -> list[AudioItem]:
    root = Path(data_dir)
    if not root.exists():
        raise FileNotFoundError(f"Data directory does not exist: {root}")
    files = sorted(p for p in root.rglob("*") if p.suffix.lower() in {".wav", ".flac", ".mp3", ".ogg"})
    items = [AudioItem(str(p), _label_from_parent(p)) for p in files]
    if not items:
        raise FileNotFoundError(f"No audio files found below {root}")
    if not any(x.label == 1 for x in items) or not any(x.label == 0 for x in items):
        raise ValueError("Both positive and negative audio samples are required.")
    return items


def stratified_split(items: Iterable[AudioItem], test_ratio: float = 0.2, seed: int = 42):
    if not 0 < test_ratio < 1:
        raise ValueError("test_ratio must be between 0 and 1")
    grouped: dict[int, list[AudioItem]] = {0: [], 1: []}
    for item in items:
        grouped[item.label].append(item)
    rng = random.Random(seed)
    train, test = [], []
    for label, group in grouped.items():
        rng.shuffle(group)
        n_test = max(1, round(len(group) * test_ratio))
        if n_test >= len(group):
            n_test = len(group) - 1
        test.extend(group[:n_test])
        train.extend(group[n_test:])
    rng.shuffle(train)
    rng.shuffle(test)
    return train, test


def write_manifest(items: Iterable[AudioItem], path: str | Path) -> None:
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f, delimiter="\t")
        writer.writerows((item.path, item.label) for item in items)


def read_manifest(path: str | Path) -> list[AudioItem]:
    with Path(path).open(encoding="utf-8") as f:
        return [AudioItem(row[0], int(row[1])) for row in csv.reader(f, delimiter="\t") if len(row) >= 2]


class KeywordDataset(Dataset):
    def __init__(
        self,
        items: list[AudioItem],
        sample_rate: int = 16_000,
        duration: float = 1.0,
        augment: bool = False,
    ) -> None:
        self.items = items
        self.sample_rate = sample_rate
        self.target_samples = round(sample_rate * duration)
        self.augment = augment
        self.extractor = MFCCExtractor(sample_rate=sample_rate)

    def __len__(self) -> int:
        return len(self.items)

    def _load(self, path: str) -> torch.Tensor:
        waveform, rate = torchaudio.load(path)
        waveform = waveform.mean(dim=0, keepdim=True)
        if rate != self.sample_rate:
            waveform = torchaudio.functional.resample(waveform, rate, self.sample_rate)
        n = waveform.shape[-1]
        if n > self.target_samples:
            start = random.randint(0, n - self.target_samples) if self.augment else (n - self.target_samples) // 2
            waveform = waveform[:, start : start + self.target_samples]
        elif n < self.target_samples:
            left = random.randint(0, self.target_samples - n) if self.augment else (self.target_samples - n) // 2
            waveform = torch.nn.functional.pad(waveform, (left, self.target_samples - n - left))
        return waveform.clamp(-1.0, 1.0)

    def _augment(self, waveform: torch.Tensor) -> torch.Tensor:
        if random.random() < 0.7:
            waveform = waveform * (10 ** (random.uniform(-6, 6) / 20))
        if random.random() < 0.5:
            snr_db = random.uniform(10, 25)
            noise = torch.randn_like(waveform)
            noise = noise / (noise.std(unbiased=False) + 1e-6)
            signal_rms = waveform.pow(2).mean().sqrt()
            waveform = waveform + noise * signal_rms / (10 ** (snr_db / 20))
        if random.random() < 0.3:
            shift = random.randint(-int(0.1 * self.sample_rate), int(0.1 * self.sample_rate))
            waveform = torch.roll(waveform, shifts=shift, dims=-1)
        return waveform.clamp(-1.0, 1.0)

    def __getitem__(self, index: int):
        item = self.items[index]
        waveform = self._load(item.path)
        if self.augment:
            waveform = self._augment(waveform)
        return self.extractor(waveform), torch.tensor(float(item.label), dtype=torch.float32)
