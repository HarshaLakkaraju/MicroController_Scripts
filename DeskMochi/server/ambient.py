import urllib.request
import urllib.parse
import json
import time
import random

# --- CONFIGURATION ---
MOCHI_URL = "http://mochi.local"   # or use raw IP: "http://192.168.1.55"
NORMAL_INTERVAL = 300              # 5 minutes (Mochi is online & working)
ERROR_INTERVAL = 3600              # 1 hour (Mochi is offline/unplugged)

ANIMATIONS = ["fly", "suspicious", "dizzy", "shy", "thinking"]
# ---------------------

print("🍡 Mochi Ambient Service started. Running in background...")

while True:
    try:
        response = urllib.request.urlopen(f"{MOCHI_URL}/status", timeout=5)

        if response.getcode() != 200:
            raise Exception(f"HTTP Error: {response.getcode()}")

        data = json.loads(response.read().decode())
        current_mode = data.get("mode", "UNKNOWN")

        if current_mode == "SITTING":
            chosen_anim = random.choice(ANIMATIONS)
            print(f"Mochi is SITTING. Triggering: {chosen_anim}")
            post_data = urllib.parse.urlencode({"type": chosen_anim}).encode()
            urllib.request.urlopen(f"{MOCHI_URL}/notify", data=post_data, timeout=5)
        else:
            print(f"Mochi is {current_mode}. Skipping animation.")

        time.sleep(NORMAL_INTERVAL)

    except Exception as e:
        print(f"Connection failed ({e}). Mochi might be offline.")
        print("Pausing for 1 hour before retrying...")
        time.sleep(ERROR_INTERVAL)
