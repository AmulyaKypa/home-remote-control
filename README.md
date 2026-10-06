# Internet-Based Remote Control of Home Appliances Using Flask

IoT prototype using **Flask + MQTT + ESP32** for remote ON/OFF control and status monitoring.

## Architecture

```text
Mobile / Laptop
      |
   Internet
      |
  Flask Web App
      |
   MQTT Broker
      |
   Internet
      |
    ESP32
      |
 Relay / LED prototype
```

## Repository structure

```text
home_remote_control/
├── app.py
├── requirements.txt
├── render.yaml
├── .env.example
├── .gitignore
├── templates/
│   └── index.html
├── static/
│   ├── style.css
│   └── app.js
└── esp32/
    └── esp32_remote_control.ino
```

## Important security rule

Do not commit `.env`, MQTT passwords, API keys, or Wi-Fi passwords. Use Render environment variables for cloud deployment and put Wi-Fi/MQTT credentials into the ESP32 sketch locally without committing the filled-in sketch.

## Local Flask test

```bash
python -m venv venv
```

Windows:

```powershell
venv\Scripts\activate
```

Linux/macOS:

```bash
source venv/bin/activate
```

Install dependencies:

```bash
pip install -r requirements.txt
```

Run Flask:

```bash
python app.py
```

Open `http://127.0.0.1:5000`.

For local MQTT testing, set `MQTT_BROKER` and `MQTT_PORT` in your environment. Cloud MQTT configuration will be added during deployment.

## Render deployment

This repository includes `render.yaml`. Render can use:

```text
Build: pip install -r requirements.txt
Start: gunicorn app:app
```

Set the following environment variables in Render using the credentials supplied by your MQTT provider:

```text
MQTT_BROKER=<cloud MQTT hostname>
MQTT_PORT=8883
MQTT_USERNAME=<MQTT username>
MQTT_PASSWORD=<MQTT password>
```

TLS support for the cloud MQTT connection will be configured as part of the cloud deployment step.

## ESP32

Open:

```text
esp32/esp32_remote_control.ino
```

The ESP32 subscribes to:

```text
home/light
home/fan
home/geyser
```

and publishes status to:

```text
home/status
```

For the final Internet deployment, `MQTT_SERVER` must be the cloud MQTT hostname. Do not use `localhost` or a private LAN IP in the ESP32 firmware for the Internet deployment.

## Prototype safety

Use LEDs/resistors or other safe low-voltage loads for the college prototype. Do **not** connect this prototype directly to 230 V/120 V mains.

## Optional future extensions

- Gas leakage notification
- Temperature/humidity monitoring
- ML-based smart door lock
- Appliance activity/history
