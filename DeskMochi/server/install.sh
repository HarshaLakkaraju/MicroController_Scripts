#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "==> Configuring journald (50M cap)..."
sudo sed -i 's/^#*SystemMaxUse=.*/SystemMaxUse=50M/' /etc/systemd/journald.conf
sudo systemctl restart systemd-journald

echo "==> Installing systemd service..."
sudo cp "$DIR/mochi.service" /etc/systemd/system/mochi.service

echo "==> Reloading systemd..."
sudo systemctl daemon-reload

echo "==> Enabling and starting mochi.service..."
sudo systemctl enable mochi.service
sudo systemctl restart mochi.service

echo "==> Status:"
systemctl status mochi.service --no-pager
