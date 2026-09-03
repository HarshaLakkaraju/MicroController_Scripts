#!/usr/bin/env python3
import urllib.request
import urllib.parse
import json
import random

# --- CONFIGURATION ---
MOCHI_URL = "http://mochi.local"
ANIMATIONS = ["fly", "suspicious", "dizzy", "shy", "thinking"]
# ---------------------

print(f"🍡 Testing live Mochi at {MOCHI_URL}...")

try:
    # 1. Fetch Status
    print("--> Querying GET /status ...")
    response = urllib.request.urlopen(f"{MOCHI_URL}/status", timeout=5)
    data = json.loads(response.read().decode())
    print(f"✅ Status Response: {data}")
    
    current_mode = data.get("mode", "UNKNOWN")
    print(f"   Current Mochi Mode: {current_mode}")

    # 2. Trigger Animation if SITTING (or forced)
    if current_mode == "SITTING":
        chosen_anim = random.choice(ANIMATIONS)
        print(f"--> Mochi is SITTING. Triggering random animation: '{chosen_anim}' ...")
        post_data = urllib.parse.urlencode({"type": chosen_anim}).encode()
        notify_res = urllib.request.urlopen(f"{MOCHI_URL}/notify", data=post_data, timeout=5)
        print(f"✅ Notification Response: {notify_res.read().decode()}")
    else:
        print(f"ℹ️ Mochi is in '{current_mode}' mode (not 'SITTING'). Skipping automatic trigger.")
        
        # Optionally test notify endpoint anyway
        test_anim = "thinking"
        print(f"--> Testing POST /notify directly with '{test_anim}' ...")
        post_data = urllib.parse.urlencode({"type": test_anim}).encode()
        notify_res = urllib.request.urlopen(f"{MOCHI_URL}/notify", data=post_data, timeout=5)
        print(f"✅ Direct Notification Response: {notify_res.read().decode()}")

    print("\n🎉 Everything is working properly!")

except Exception as e:
    print(f"\n❌ Error connecting to Mochi: {e}")
