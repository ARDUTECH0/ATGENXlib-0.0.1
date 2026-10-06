import json
import requests
from websocket import WebSocketApp

HUB_IP = "192.168.1.10"
HTTP_MANIFEST = f"http://{HUB_IP}/atgenx/manifest"
WS_URL = f"ws://{HUB_IP}:8080"

TOKEN = ""  # لو عندك token

def pick_first_relay(manifest: dict):
    for d in manifest.get("devices", []):
        if d.get("type") == "relay":
            return d.get("id")
    return None

def main():
    print("🌐 Fetching manifest:", HTTP_MANIFEST)
    m = requests.get(HTTP_MANIFEST, timeout=3).json()
    relay_id = pick_first_relay(m)
    print("🎯 relay:", relay_id)

    def on_open(ws):
        print("✅ WS connected:", WS_URL)
        payload = {"to": relay_id, "seq": 1, "cmd": "toggle"}
        if TOKEN:
            payload["token"] = TOKEN
        ws.send(json.dumps(payload))
        print("➡️ sent:", payload)

    def on_message(ws, msg):
        print("📩", msg)
        # اقفل بعد أول رد
        ws.close()

    def on_error(ws, err):
        print("❌", err)

    def on_close(ws, code, reason):
        print("🔌 closed:", code, reason)

    WebSocketApp(WS_URL, on_open=on_open, on_message=on_message, on_error=on_error, on_close=on_close)\
        .run_forever(ping_interval=15, ping_timeout=5)

if __name__ == "__main__":
    main()