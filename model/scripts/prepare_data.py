#!/usr/bin/env python
"""Create deterministic train/test manifests from positive and negative folders."""
from __future__ import annotations

import argparse
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from keyword_spotting.data import scan_audio, stratified_split, write_manifest


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data-dir", default="data", help="Folder containing positive/ and negative/")
    parser.add_argument("--output-dir", default="data/manifests")
    parser.add_argument("--test-ratio", type=float, default=0.2)
    parser.add_argument("--seed", type=int, default=42)
    args = parser.parse_args()
    items = scan_audio(args.data_dir)
    train, test = stratified_split(items, args.test_ratio, args.seed)
    out = Path(args.output_dir)
    write_manifest(train, out / "train.tsv")
    write_manifest(test, out / "test.tsv")
    print(f"total={len(items)} train={len(train)} test={len(test)}")
    print(f"positive: train={sum(x.label for x in train)} test={sum(x.label for x in test)}")
    print(f"manifests written to {out}")


if __name__ == "__main__":
    main()
