# server/ — Home Server Scripts

This folder contains everything that runs on your **home Linux server**.
Completely separate from the ESP32 firmware in the root of this repo.

## Files

| File | Purpose |
|------|---------|
| `ambient.py` | Background script — polls Mochi every 5 mins, triggers random animations |
| `mochi.service` | systemd service unit — keeps `ambient.py` running forever, survives reboots |
| `install.sh` | One-command installer — copies service, caps journal, enables & starts |
| `test_live.py` | Test against a real connected Mochi device |
| `test_mock.py` | Test script logic without needing a Mochi device |

---

## Quick Install

```bash
# 1. Copy this server/ folder to your Linux server
# 2. Edit mochi.service — replace 'haxxha' with your actual Linux username
#    and update the paths if you placed the folder somewhere other than ~/server
# 3. Edit ambient.py — set MOCHI_URL to your Mochi's IP if mochi.local doesn't resolve
# 4. Run the installer
chmod +x install.sh
./install.sh
```

## Manage the Service

```bash
systemctl status mochi.service        # Is it running?
journalctl -u mochi.service -f        # Watch live logs
sudo systemctl restart mochi.service  # Restart after config change
sudo systemctl stop mochi.service     # Stop it
```

## mDNS Fix (if mochi.local doesn't resolve on Linux)

```bash
sudo apt-get update && sudo apt-get install -y avahi-daemon
sudo systemctl restart mochi.service
```

Or edit `ambient.py` and use the raw IP:
```python
MOCHI_URL = "http://192.168.1.55"   # ← your Mochi's actual IP
```
