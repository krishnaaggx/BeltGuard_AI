"""
BeltGuard AI — Inference Helper
================================
Import this in app.py to run predictions on incoming sensor data.

Usage:
    from predict import BeltGuardPredictor
    predictor = BeltGuardPredictor()
    result = predictor.predict({
        "vibration_rms": 1.0,
        "vibration_peak": 1.35,
        "vibration_crest_factor": 1.35,
        "temp_surface_c": 38.0,
        "temp_bearing_c": 42.0,
        "belt_speed_mps": 2.0,
        "load_kg": 100.0,
        "ir_offset_mm": 0.0
    })
    # result = {"label": "NORMAL", "confidence": 0.97, "probabilities": {...}}
"""

import pickle
import numpy as np
import os

FEATURES = [
    "vibration_rms",
    "vibration_peak",
    "vibration_crest_factor",
    "temp_surface_c",
    "temp_bearing_c",
    "belt_speed_mps",
    "load_kg",
    "ir_offset_mm",
]

MODEL_DIR = os.path.dirname(os.path.abspath(__file__))


class BeltGuardPredictor:
    def __init__(self):
        with open(os.path.join(MODEL_DIR, "model.pkl"),         "rb") as f:
            self.model = pickle.load(f)
        with open(os.path.join(MODEL_DIR, "scaler.pkl"),        "rb") as f:
            self.scaler = pickle.load(f)
        with open(os.path.join(MODEL_DIR, "label_encoder.pkl"), "rb") as f:
            self.le = pickle.load(f)
        print("BeltGuardPredictor loaded.")

    def predict(self, sensor_dict: dict) -> dict:
        """
        sensor_dict: dict with keys matching FEATURES
        Returns: {"label": str, "confidence": float, "probabilities": dict}
        """
        # Extract features in correct order, default 0.0 if missing
        x = np.array([[sensor_dict.get(f, 0.0) for f in FEATURES]])
        x_scaled = self.scaler.transform(x)

        pred_enc   = self.model.predict(x_scaled)[0]
        pred_proba = self.model.predict_proba(x_scaled)[0]

        label      = self.le.inverse_transform([pred_enc])[0]
        confidence = float(pred_proba[pred_enc])

        probabilities = {
            cls: float(prob)
            for cls, prob in zip(self.le.classes_, pred_proba)
        }

        return {
            "label":         label,
            "confidence":    round(confidence, 4),
            "probabilities": probabilities
        }
