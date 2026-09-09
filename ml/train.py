"""
BeltGuard AI — ML Training Script
===================================
Trains a Random Forest classifier on synthetic belt sensor data.
When real data is available from InfluxDB, replace generate_synthetic_data()
with load_real_data() — everything else stays the same.

Output files:
  model.pkl         — trained Random Forest classifier
  scaler.pkl        — StandardScaler (must be used on inference data too)
  label_encoder.pkl — LabelEncoder (maps class names ↔ integers)
  training_data.csv — synthetic dataset used for training (for reference)
"""

import numpy as np
import pandas as pd
import pickle
import os
from sklearn.ensemble import RandomForestClassifier
from sklearn.preprocessing import StandardScaler, LabelEncoder
from sklearn.model_selection import train_test_split
from sklearn.metrics import classification_report, confusion_matrix

# ── Reproducibility ──────────────────────────────────────────────────────────
np.random.seed(42)

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

# ── Synthetic data generation ─────────────────────────────────────────────────
def generate_class(n, **kwargs):
    """Generate n rows with specified mean±std for each feature."""
    rows = {}
    for feat in FEATURES:
        mean, std = kwargs.get(feat, (0.0, 0.01))
        rows[feat] = np.random.normal(mean, std, n).tolist()
    return pd.DataFrame(rows)


def generate_synthetic_data(n_per_class=600):
    """
    Realistic synthetic ranges per fault type.
    Replace this function with InfluxDB data pull when hardware is ready.
    """

    # ── NORMAL ───────────────────────────────────────────────────────────────
    normal = generate_class(n_per_class,
        vibration_rms          = (1.0,  0.10),
        vibration_peak         = (1.35, 0.12),
        vibration_crest_factor = (1.35, 0.08),   # healthy: ~1.2–1.5
        temp_surface_c         = (38.0, 3.0),
        temp_bearing_c         = (42.0, 3.0),
        belt_speed_mps         = (2.0,  0.10),
        load_kg                = (100,  10.0),
        ir_offset_mm           = (0.0,  1.0),    # belt centred
    )
    normal["label"] = "NORMAL"

    # ── MISALIGNMENT ─────────────────────────────────────────────────────────
    # Belt drifts sideways → IR offset spikes, bearing temp rises slightly
    misalignment = generate_class(n_per_class,
        vibration_rms          = (1.4,  0.15),
        vibration_peak         = (1.9,  0.20),
        vibration_crest_factor = (1.55, 0.12),
        temp_surface_c         = (42.0, 3.0),
        temp_bearing_c         = (58.0, 5.0),
        belt_speed_mps         = (1.9,  0.10),
        load_kg                = (100,  10.0),
        ir_offset_mm           = (11.0, 2.0),    # key indicator
    )
    misalignment["label"] = "MISALIGNMENT"

    # ── OVERLOAD ─────────────────────────────────────────────────────────────
    # Too much material on belt → load_kg high, speed drops, temps rise
    overload = generate_class(n_per_class,
        vibration_rms          = (2.0,  0.25),
        vibration_peak         = (2.8,  0.30),
        vibration_crest_factor = (1.50, 0.10),
        temp_surface_c         = (65.0, 6.0),
        temp_bearing_c         = (60.0, 5.0),
        belt_speed_mps         = (1.1,  0.15),   # slows under load
        load_kg                = (380,  30.0),    # key indicator
        ir_offset_mm           = (1.0,  1.5),
    )
    overload["label"] = "OVERLOAD"

    # ── BEARING_FAULT ─────────────────────────────────────────────────────────
    # Worn bearing → high vibration, high crest factor, hot bearing
    bearing_fault = generate_class(n_per_class,
        vibration_rms          = (3.5,  0.40),
        vibration_peak         = (8.0,  0.80),
        vibration_crest_factor = (4.5,  0.50),   # key indicator: peak/rms ratio spikes
        temp_surface_c         = (45.0, 4.0),
        temp_bearing_c         = (85.0, 8.0),    # key indicator: bearing runs hot
        belt_speed_mps         = (1.8,  0.15),
        load_kg                = (105,  10.0),
        ir_offset_mm           = (1.5,  1.5),
    )
    bearing_fault["label"] = "BEARING_FAULT"

    # ── SPLICE_FAULT ──────────────────────────────────────────────────────────
    # Damaged joint → impact vibration each revolution, surface heat at joint
    splice_fault = generate_class(n_per_class,
        vibration_rms          = (2.2,  0.30),
        vibration_peak         = (5.5,  0.60),
        vibration_crest_factor = (2.8,  0.35),
        temp_surface_c         = (72.0, 7.0),    # key indicator: joint friction heat
        temp_bearing_c         = (55.0, 5.0),
        belt_speed_mps         = (1.85, 0.12),
        load_kg                = (102,  10.0),
        ir_offset_mm           = (2.0,  2.0),
    )
    splice_fault["label"] = "SPLICE_FAULT"

    df = pd.concat([normal, misalignment, overload, bearing_fault, splice_fault],
                   ignore_index=True)
    df = df.sample(frac=1, random_state=42).reset_index(drop=True)  # shuffle
    return df


# ── Real data loader (use this when hardware is ready) ───────────────────────
def load_real_data(csv_path="training_data_real.csv"):
    """
    When real data is collected, export from InfluxDB to CSV and call this.
    CSV must have columns matching FEATURES + 'label'.
    """
    df = pd.read_csv(csv_path)
    missing = [f for f in FEATURES + ["label"] if f not in df.columns]
    if missing:
        raise ValueError(f"CSV missing columns: {missing}")
    return df[FEATURES + ["label"]].dropna()


# ── Training ─────────────────────────────────────────────────────────────────
def train(df):
    print(f"\nDataset shape : {df.shape}")
    print(f"Class distribution:\n{df['label'].value_counts()}\n")

    X = df[FEATURES].values
    y = df["label"].values

    # Encode labels to integers
    le = LabelEncoder()
    y_enc = le.fit_transform(y)
    print(f"Classes : {list(le.classes_)}")

    # Scale features
    scaler = StandardScaler()
    X_scaled = scaler.fit_transform(X)

    # Train / test split — 80/20
    X_train, X_test, y_train, y_test = train_test_split(
        X_scaled, y_enc, test_size=0.2, random_state=42, stratify=y_enc
    )

    # Random Forest
    model = RandomForestClassifier(
        n_estimators=150,
        max_depth=None,
        min_samples_split=4,
        class_weight="balanced",   # handles unequal class sizes in real data
        random_state=42,
        n_jobs=-1
    )
    model.fit(X_train, y_train)

    # Evaluate
    y_pred = model.predict(X_test)
    print("\n── Classification Report ──────────────────────────────")
    print(classification_report(y_test, y_pred, target_names=le.classes_))

    print("── Confusion Matrix ───────────────────────────────────")
    cm = confusion_matrix(y_test, y_pred)
    cm_df = pd.DataFrame(cm, index=le.classes_, columns=le.classes_)
    print(cm_df)

    print("\n── Feature Importance ─────────────────────────────────")
    fi = sorted(zip(FEATURES, model.feature_importances_), key=lambda x: -x[1])
    for feat, imp in fi:
        bar = "█" * int(imp * 40)
        print(f"  {feat:<30} {imp:.4f}  {bar}")

    return model, scaler, le


# ── Save artefacts ────────────────────────────────────────────────────────────
def save(model, scaler, le, out_dir="."):
    os.makedirs(out_dir, exist_ok=True)
    with open(os.path.join(out_dir, "model.pkl"),         "wb") as f: pickle.dump(model, f)
    with open(os.path.join(out_dir, "scaler.pkl"),        "wb") as f: pickle.dump(scaler, f)
    with open(os.path.join(out_dir, "label_encoder.pkl"), "wb") as f: pickle.dump(le, f)
    print(f"\nSaved: model.pkl  scaler.pkl  label_encoder.pkl → {out_dir}/")


# ── Main ──────────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    print("BeltGuard AI — Training on synthetic data")
    print("(Replace generate_synthetic_data() with load_real_data() when hardware is ready)\n")

    df = generate_synthetic_data(n_per_class=600)
    df.to_csv("training_data.csv", index=False)
    print("Saved training_data.csv (3000 rows)")

    model, scaler, le = train(df)
    save(model, scaler, le, out_dir=".")
