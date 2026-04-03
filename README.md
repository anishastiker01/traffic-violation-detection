# 🚦 Traffic Signal Violation Detection System

An **ESP32-CAM** based smart traffic enforcement system that automatically detects red-light violations, captures photo evidence, and sends it to a central server with a live web dashboard.

---

## 📌 How It Works

1. **IR sensor** detects if a vehicle is present at a red light junction
2. A **5-second grace period** is given — for vehicles that were too close to stop safely
3. If the vehicle is **still present after 5 seconds**, it confirms a violation
4. The ESP32-CAM **captures a photo** (with flash)
5. The image is **sent to the server** via WiFi (HTTP POST) with location ID and timestamp
6. The server **logs the violation** and displays it on a live **web dashboard**

---

## 📁 Project Structure

```
traffic-violation-detection/
│
├── esp32-cam/
│   └── traffic_violation_detector.ino   # Arduino sketch for ESP32-CAM
│
├── server/
│   ├── server.py                        # Flask server (receives photos, serves dashboard)
│   └── requirements.txt                 # Python dependencies
│
├── dashboard/
│   └── templates/
│       └── dashboard.html               # Live violation dashboard (auto-refreshes)
│
├── docs/
│   └── WIRING.md                        # Hardware wiring guide
│
└── README.md
```

---

## 🛠️ Hardware Required

| Component              | Purpose                        |
|------------------------|--------------------------------|
| ESP32-CAM (AI-Thinker) | Capture and send photos        |
| IR Proximity Sensor    | Detect vehicle presence        |
| FTDI Programmer        | Upload code to ESP32-CAM       |
| 5V 2A Power Supply     | Power the system               |
| Traffic light signal   | Input to detect red light      |

📐 See full wiring guide → [`docs/WIRING.md`](docs/WIRING.md)

---

## 🚀 Setup Instructions

### 1. ESP32-CAM (Arduino)

1. Install **Arduino IDE** → add ESP32 board support
2. Open `esp32-cam/traffic_violation_detector.ino`
3. Set your WiFi credentials and server IP:
   ```cpp
   const char* WIFI_SSID  = "YOUR_WIFI_SSID";
   const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
   const char* SERVER_URL = "http://YOUR_SERVER_IP:5000/upload";
   ```
4. Connect FTDI programmer (see wiring guide), short GPIO0 to GND
5. Upload the sketch
6. Remove GPIO0-GND wire and power cycle

---

### 2. Python Server

```bash
cd server
pip install -r requirements.txt
python server.py
```

Server runs at: `http://0.0.0.0:5000`

---

### 3. Dashboard

Open your browser → `http://YOUR_SERVER_IP:5000`

- View all violations
- Click any thumbnail to expand the photo
- Auto-refreshes every 30 seconds

---

## 📡 API Endpoints

| Method | Endpoint          | Description                        |
|--------|-------------------|------------------------------------|
| POST   | `/upload`         | Receive image from ESP32-CAM       |
| GET    | `/violations`     | Get all violations (JSON)          |
| GET    | `/stats`          | Summary stats (JSON)               |
| GET    | `/image/<file>`   | Serve violation image              |
| GET    | `/`               | Live web dashboard                 |

---

## 🔮 Future Improvements

- [ ] License plate OCR using OpenCV
- [ ] SMS/email alerts to traffic authority
- [ ] Multiple camera junction support
- [ ] Export violations to PDF/Excel report
- [ ] AI-based vehicle type classification

---

## 👤 Author

Made with ❤️ using ESP32-CAM + Flask

---

## 📄 License

MIT License — Free to use and modify
