import json
import os
import threading
from datetime import datetime

from flask import Flask, jsonify, render_template, request
import paho.mqtt.client as mqtt

app = Flask(__name__)

MQTT_BROKER = os.getenv("MQTT_BROKER", "localhost")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))
MQTT_USERNAME = os.getenv("MQTT_USERNAME", "")
MQTT_PASSWORD = os.getenv("MQTT_PASSWORD", "")

TOPICS = {
    "light": "home/light",
    "fan": "home/fan",
    "geyser": "home/geyser",
    "status": "home/status",
}

state_lock = threading.Lock()
device_state = {
    "connected": False,
    "last_update": None,
    "last_command": None,
    "devices": {
        "light": "OFF",
        "fan": "OFF",
        "geyser": "OFF",
    },
    "sensors": {
        "temperature": None,
        "humidity": None,
        "gas": "NORMAL",
    },
}

def mqtt_on_connect(client, userdata, flags, reason_code, properties=None):
    connected = reason_code == 0
    with state_lock:
        device_state["connected"] = connected
    if connected:
        client.subscribe(TOPICS["status"])
        print(f"[MQTT] Connected to {MQTT_BROKER}:{MQTT_PORT}")

def mqtt_on_disconnect(client, userdata, disconnect_flags, reason_code, properties=None):
    with state_lock:
        device_state["connected"] = False
    print(f"[MQTT] Disconnected: {reason_code}")

def mqtt_on_message(client, userdata, msg):
    if msg.topic != TOPICS["status"]:
        return

    try:
        payload = json.loads(msg.payload.decode("utf-8"))
    except (json.JSONDecodeError, UnicodeDecodeError):
        print("[MQTT] Invalid status payload:", msg.payload)
        return

    with state_lock:
        devices = payload.get("devices", {})
        sensors = payload.get("sensors", {})

        for name in ("light", "fan", "geyser"):
            if name in devices:
                device_state["devices"][name] = str(devices[name]).upper()

        for name in ("temperature", "humidity", "gas"):
            if name in sensors:
                device_state["sensors"][name] = sensors[name]

        device_state["last_update"] = datetime.now().isoformat(timespec="seconds")

mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="flask-home-dashboard")

if MQTT_USERNAME:
    mqtt_client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)

mqtt_client.on_connect = mqtt_on_connect
mqtt_client.on_disconnect = mqtt_on_disconnect
mqtt_client.on_message = mqtt_on_message

try:
    mqtt_client.connect(MQTT_BROKER, MQTT_PORT, keepalive=60)
    mqtt_client.loop_start()
except Exception as exc:
    print(f"[MQTT] Initial connection failed: {exc}")
    print("Start the broker and restart Flask, or set MQTT_BROKER to your broker address.")

@app.get("/")
def index():
    return render_template("index.html")

@app.get("/api/status")
def api_status():
    with state_lock:
        return jsonify(device_state)

@app.post("/api/device/<device>/<action>")
def control_device(device, action):
    allowed_devices = {"light", "fan", "geyser"}
    allowed_actions = {"ON", "OFF"}

    device = device.lower()
    action = action.upper()

    if device not in allowed_devices:
        return jsonify({"ok": False, "error": "Unknown device"}), 400

    if action not in allowed_actions:
        return jsonify({"ok": False, "error": "Action must be ON or OFF"}), 400

    topic = TOPICS[device]

    if not mqtt_client.is_connected():
        return jsonify({"ok": False, "error": "MQTT broker is not connected"}), 503

    info = mqtt_client.publish(topic, action, qos=1, retain=False)

    if info.rc != mqtt.MQTT_ERR_SUCCESS:
        return jsonify({"ok": False, "error": "MQTT publish failed"}), 500

    with state_lock:
        device_state["last_command"] = {
            "device": device,
            "action": action,
            "time": datetime.now().isoformat(timespec="seconds"),
        }

    return jsonify({
        "ok": True,
        "device": device,
        "action": action,
        "topic": topic,
    })

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
