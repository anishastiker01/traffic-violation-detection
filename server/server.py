"""
Traffic Violation Detection System - Server
============================================
Receives photos from ESP32-CAM modules,
saves them with metadata, and logs violations
to a SQLite database.

Run: python server.py
"""

import os
import sqlite3
from datetime import datetime
from flask import Flask, request, jsonify, render_template, send_from_directory

app = Flask(__name__)

# ─── Config ──────────────────────────────────────────────────────────────────
UPLOAD_FOLDER = "violations"
DB_PATH       = "violations.db"
PORT          = 5000
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

# ─── Database Setup ───────────────────────────────────────────────────────────
def init_db():
    conn = sqlite3.connect(DB_PATH)
    conn.execute("""
        CREATE TABLE IF NOT EXISTS violations (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            location_id TEXT    NOT NULL,
            timestamp   TEXT    NOT NULL,
            filename    TEXT    NOT NULL,
            received_at TEXT    NOT NULL
        )
    """)
    conn.commit()
    conn.close()

init_db()

# ─── Upload Endpoint ──────────────────────────────────────────────────────────
@app.route("/upload", methods=["POST"])
def upload():
    location_id = request.headers.get("X-Location-ID", "UNKNOWN")
    timestamp   = request.headers.get("X-Timestamp", datetime.now().strftime("%Y-%m-%d_%H-%M-%S"))
    received_at = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    image_data  = request.data
    if not image_data:
        return jsonify({"error": "No image data received"}), 400

    # Save image
    filename = f"{location_id}_{timestamp}.jpg"
    filepath = os.path.join(UPLOAD_FOLDER, filename)
    with open(filepath, "wb") as f:
        f.write(image_data)

    # Log to database
    conn = sqlite3.connect(DB_PATH)
    conn.execute(
        "INSERT INTO violations (location_id, timestamp, filename, received_at) VALUES (?, ?, ?, ?)",
        (location_id, timestamp, filename, received_at)
    )
    conn.commit()
    conn.close()

    print(f"[{received_at}] Violation from {location_id} saved → {filename}")
    return jsonify({"status": "success", "file": filename}), 200


# ─── View All Violations (JSON) ───────────────────────────────────────────────
@app.route("/violations", methods=["GET"])
def get_violations():
    conn   = sqlite3.connect(DB_PATH)
    cursor = conn.execute("SELECT * FROM violations ORDER BY id DESC")
    rows   = cursor.fetchall()
    conn.close()

    violations = []
    for row in rows:
        violations.append({
            "id":          row[0],
            "location_id": row[1],
            "timestamp":   row[2],
            "filename":    row[3],
            "received_at": row[4]
        })
    return jsonify(violations)


# ─── Serve Violation Images ───────────────────────────────────────────────────
@app.route("/image/<filename>")
def serve_image(filename):
    return send_from_directory(UPLOAD_FOLDER, filename)


# ─── Dashboard ────────────────────────────────────────────────────────────────
@app.route("/")
def dashboard():
    return render_template("dashboard.html")


# ─── Stats Endpoint ───────────────────────────────────────────────────────────
@app.route("/stats")
def stats():
    conn  = sqlite3.connect(DB_PATH)
    total = conn.execute("SELECT COUNT(*) FROM violations").fetchone()[0]
    today = conn.execute(
        "SELECT COUNT(*) FROM violations WHERE received_at LIKE ?",
        (datetime.now().strftime("%Y-%m-%d") + "%",)
    ).fetchone()[0]
    locations = conn.execute(
        "SELECT location_id, COUNT(*) as count FROM violations GROUP BY location_id ORDER BY count DESC"
    ).fetchall()
    conn.close()

    return jsonify({
        "total":     total,
        "today":     today,
        "locations": [{"location": r[0], "count": r[1]} for r in locations]
    })


# ─── Run ──────────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    print(f"[SERVER] Starting on http://0.0.0.0:{PORT}")
    app.run(host="0.0.0.0", port=PORT, debug=True)
