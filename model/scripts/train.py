#!/usr/bin/env python
from __future__ import annotations

import argparse
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import torch
from torch import nn
from torch.utils.data import DataLoader

from keyword_spotting.data import KeywordDataset, read_manifest, scan_audio, stratified_split
from keyword_spotting.model import KeywordCNN
from keyword_spotting.utils import save_json, seed_everything


def evaluate(model, loader, criterion, device):
    model.eval()
    total_loss, n, correct = 0.0, 0, 0
    with torch.no_grad():
        for x, y in loader:
            x, y = x.to(device), y.to(device)
            logits = model(x)
            total_loss += criterion(logits, y).item() * len(y)
            correct += ((torch.sigmoid(logits) >= 0.5) == (y >= 0.5)).sum().item()
            n += len(y)
    return total_loss / max(1, n), correct / max(1, n)


def main() -> None:
    p = argparse.ArgumentParser(description="Train the MFCC CNN keyword detector")
    p.add_argument("--data-dir", default="data")
    p.add_argument("--train-manifest", default=None)
    p.add_argument("--test-manifest", default=None)
    p.add_argument("--checkpoint", default="checkpoints/best.pt")
    p.add_argument("--epochs", type=int, default=100)
    p.add_argument("--batch-size", type=int, default=32)
    p.add_argument("--lr", type=float, default=1e-3)
    p.add_argument("--patience", type=int, default=15)
    p.add_argument("--seed", type=int, default=42)
    p.add_argument("--device", default="auto", choices=["auto", "cpu", "cuda"])
    args = p.parse_args()
    seed_everything(args.seed)
    device = torch.device("cuda" if args.device == "auto" and torch.cuda.is_available() else args.device if args.device != "auto" else "cpu")

    if args.train_manifest and args.test_manifest:
        train_items, test_items = read_manifest(args.train_manifest), read_manifest(args.test_manifest)
    else:
        train_items, test_items = stratified_split(scan_audio(args.data_dir), seed=args.seed)
    if not train_items or not test_items:
        raise ValueError("Training and test sets must both contain samples")
    train_ds = KeywordDataset(train_items, augment=True)
    test_ds = KeywordDataset(test_items, augment=False)
    train_loader = DataLoader(train_ds, batch_size=args.batch_size, shuffle=True, num_workers=0)
    test_loader = DataLoader(test_ds, batch_size=args.batch_size, shuffle=False, num_workers=0)

    model = KeywordCNN().to(device)
    positives = sum(x.label for x in train_items)
    negatives = len(train_items) - positives
    pos_weight = torch.tensor([negatives / max(1, positives)], device=device)
    criterion = nn.BCEWithLogitsLoss(pos_weight=pos_weight)
    optimizer = torch.optim.Adam(model.parameters(), lr=args.lr)
    scheduler = torch.optim.lr_scheduler.ReduceLROnPlateau(optimizer, mode="min", factor=0.5, patience=5)
    checkpoint = Path(args.checkpoint)
    checkpoint.parent.mkdir(parents=True, exist_ok=True)
    history, best_loss, stale = [], float("inf"), 0

    print(f"device={device} train={len(train_ds)} test={len(test_ds)}")
    for epoch in range(1, args.epochs + 1):
        model.train()
        running_loss, seen, correct = 0.0, 0, 0
        for x, y in train_loader:
            x, y = x.to(device), y.to(device)
            optimizer.zero_grad(set_to_none=True)
            logits = model(x)
            loss = criterion(logits, y)
            loss.backward()
            optimizer.step()
            running_loss += loss.item() * len(y)
            correct += ((torch.sigmoid(logits) >= 0.5) == (y >= 0.5)).sum().item()
            seen += len(y)
        train_loss, train_acc = running_loss / max(1, seen), correct / max(1, seen)
        val_loss, val_acc = evaluate(model, test_loader, criterion, device)
        scheduler.step(val_loss)
        row = {"epoch": epoch, "train_loss": train_loss, "train_accuracy": train_acc,
               "val_loss": val_loss, "val_accuracy": val_acc, "lr": optimizer.param_groups[0]["lr"]}
        history.append(row)
        print("epoch={epoch:03d} train_loss={train_loss:.4f} train_acc={train_accuracy:.3f} val_loss={val_loss:.4f} val_acc={val_accuracy:.3f}".format(**row))
        if val_loss < best_loss - 1e-5:
            best_loss, stale = val_loss, 0
            torch.save({"model": model.state_dict(), "threshold": 0.5, "sample_rate": 16000,
                        "history": history, "model_name": "KeywordCNN"}, checkpoint)
        else:
            stale += 1
            if stale >= args.patience:
                print(f"early stopping at epoch {epoch}")
                break
    save_json(history, checkpoint.with_suffix(".json"))
    print(f"best checkpoint: {checkpoint}")


if __name__ == "__main__":
    main()
