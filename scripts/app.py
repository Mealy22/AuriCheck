from flask import Flask, jsonify, render_template, request
import os
import numpy as np
import pandas as pd
from scipy.signal import find_peaks
import matplotlib.pyplot as plt

app = Flask(__name__)

@app.route('/')
def index():
    return render_template('Intro.html')

@app.route('/pagina_principal')
def pagina_principal():
    return render_template('pagina_principal.html')

def detectar_arritmias(data):
    data = np.array([list(map(float, line.split(','))) for line in data.split('\n') if line])
    datos = data[:, 2]

    fs = 120  # Frecuencia de muestreo

    def detect_r_peaks(ecg_signal, fs):
        peaks, _ = find_peaks(ecg_signal, height=(0.4, 0.9))
        return peaks

    def detect_p_peaks(ecg_signal, fs):
        peaks, _ = find_peaks(ecg_signal, height=(0.01, 0.18))
        return peaks

    def calculate_intervals(peaks, fs):
        intervals = np.diff(peaks) / fs  # Convertir a segundos
        return intervals

    def identify_arrhythmias(p_peaks, p_wave_amplitudes, p_count_threshold=1):
        arrhythmias = []
        for i in range(len(p_peaks) - p_count_threshold):
            if p_peaks[i + p_count_threshold] - p_peaks[i] < fs:
                arrhythmias.append(i)
        return arrhythmias
    
    def calculate_bpm(r_peaks, fs):
        rr_intervals = np.diff(r_peaks) / fs  # Convertir a segundos
        valid_intervals = rr_intervals[(rr_intervals > 0.4) & (rr_intervals < 2.0)]  # Filtrar intervalos no realistas
        bpm = 60 / np.median(valid_intervals)  # Usar mediana en lugar de media
        return bpm

    p_peaks = detect_p_peaks(datos, fs)
    r_peaks = detect_r_peaks(datos, fs)
    rr_intervals = calculate_intervals(r_peaks, fs)
    bpm = calculate_bpm(r_peaks, fs)

    p_amplitud = datos[p_peaks]

    arrhythmias = identify_arrhythmias(p_peaks, p_amplitud)

    plt.figure(figsize=(10, 6))
    plt.plot(np.arange(len(datos)) / fs, datos, label='ECG Signal')
    plt.plot(r_peaks / fs, datos[r_peaks], 'ro', label='R Peaks')
    plt.scatter(p_peaks / fs, datos[p_peaks], color='green', label='P Waves')
    for j in arrhythmias:
        plt.axvline(x=p_peaks[j] / fs, color='purple', linestyle='--', label='Arrhythmia' if j == 0 else "")
    plt.title('ECG Signal with R Peaks')
    plt.xlabel('Time (s)')
    plt.ylabel('Amplitude (mV)')
    plt.legend()
    plt.grid(True)
    plt.show()

    resultado = {
        "arritmia_detectada": len(arrhythmias) > 2,
        "detalles": "Posible caso de FIBRILACIÓN AURICULAR" if len(arrhythmias) > 2 else "Pocas arritmias detectadas",
        "BPM": bpm
    }
    return resultado

@app.route('/upload', methods=['POST'])
def upload_data():
    data = request.get_data(as_text=True)
    resultado = detectar_arritmias(data)
    return jsonify(resultado)

if __name__ == '__main__':
    app.run(debug=True)
