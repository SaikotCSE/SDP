import tkinter as tk
from tkinter import messagebox
import socket
import threading
import time

# ESP32 IP and port configuration
ESP32_IP = "192.168.137.75"  # Replace with your ESP32's IP address
ESP32_PORT = 8080           # Replace with the port number used by your ESP32
 
# Global variables
car_speed = "0.0 m/s"
stop_event = threading.Event()
warning_issued = False
speed_exceeded_time = None
SPEED_LIMIT = 30.0  # Speed limit in m/s
WARNING_DURATION = 5  # Time in seconds before stopping the car

# Function to send commands to ESP32
def send_command(command):
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
            client_socket.connect((ESP32_IP, ESP32_PORT))
            client_socket.sendall((command + "\n").encode())
            print(f"Sent command: {command}")
    except socket.error as e:
        messagebox.showerror("Error", f"Failed to send command: {e}")

# Movement control functions
def move_forward(): send_command("forward")
def move_backward(): send_command("backward")
def turn_left(): send_command("right")
def turn_right(): send_command("left")
def stop(): send_command("stop")

def increase_speed(): send_command("increase_speed")
def decrease_speed(): send_command("decrease_speed")

# Function to receive and update speed
def receive_speed():
    global car_speed, warning_issued, speed_exceeded_time
    while not stop_event.is_set():
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
                client_socket.connect((ESP32_IP, ESP32_PORT))
                client_socket.sendall("get_speed\n".encode())
                speed_data = client_socket.recv(1024).decode().strip()
                
                if speed_data:
                    car_speed = speed_data
                    speed_label.config(text=f"Speed: {car_speed} ms")
                    
                    # Speed limit logic
                    try:
                        speed_value = float(car_speed.split()[0])
                        if speed_value > SPEED_LIMIT:
                            if not warning_issued:
                                messagebox.showwarning("Speed Warning", "Speed limit exceeded! Slow down.")
                                warning_issued = True
                                speed_exceeded_time = time.time()
                            elif time.time() - speed_exceeded_time >= WARNING_DURATION:
                                stop()
                                messagebox.showerror("Speed Violation", "Speed remained above limit. Stopping car.")
                                warning_issued = False  # Reset warning
                        else:
                            warning_issued = False  # Reset if speed is reduced
                            speed_exceeded_time = None
                    except ValueError:
                        print("Invalid speed data received.")
        except socket.error as e:
            print(f"Error receiving speed data: {e}")
        
        stop_event.wait(1)  # Update speed every 1 second

# GUI Setup
root = tk.Tk()
root.title("ESP32 Car Controller")
root.geometry("400x500")

# Speed Display
speed_label = tk.Label(root, text=f"Speed: {car_speed}", font=("Arial", 14, "bold"))
speed_label.pack(pady=10)

# Movement Controls
movement_frame = tk.LabelFrame(root, text="Movement Controls", font=("Arial", 12, "bold"), padx=10, pady=10)
movement_frame.pack(pady=20)

tk.Button(movement_frame, text="↑", fg='white', bg='#007acc', command=move_forward, width=8, height=2).grid(row=0, column=1, pady=5)
tk.Button(movement_frame, text="←", fg='white', bg='#007acc', command=turn_left, width=8, height=2).grid(row=1, column=0, padx=5)
tk.Button(movement_frame, text="Stop", fg='white', bg='#d9534f', command=stop, width=8, height=2).grid(row=1, column=1, pady=5)
tk.Button(movement_frame, text="→", fg='white', bg='#007acc', command=turn_right, width=8, height=2).grid(row=1, column=2, padx=5)
tk.Button(movement_frame, text="↓", fg='white', bg='#007acc', command=move_backward, width=8, height=2).grid(row=2, column=1, pady=5)

# Speed Controls
speed_frame = tk.LabelFrame(root, text="Speed Control", font=("Arial", 12, "bold"), padx=10, pady=10)
speed_frame.pack(pady=10)
tk.Button(speed_frame, text="++ Speed", fg='white', bg='#5cb85c', command=increase_speed, width=20, height=2).pack(pady=5)
tk.Button(speed_frame, text="-- Speed", fg='white', bg='#f0ad4e', command=decrease_speed, width=20, height=2).pack(pady=5)

# Start the speed update thread
speed_thread = threading.Thread(target=receive_speed, daemon=True)
speed_thread.start()

# Stop thread on close
def on_close():
    stop_event.set()
    root.destroy()

root.protocol("WM_DELETE_WINDOW", on_close)
root.mainloop()
