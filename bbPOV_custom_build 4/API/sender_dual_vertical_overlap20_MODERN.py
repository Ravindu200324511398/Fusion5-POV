"""
bbPOV-P Dual Vertical Sender — modern UI edition
=================================================

Same capture / streaming engine as the original Tkinter version
(sender_dual_vertical_overlap20_FINAL.py), restyled with CustomTkinter
for a modern, dark, rounded-corner interface that matches the bbPOV
web dashboard's look (deep void background, cyan/magenta accent).

Fan 1 / TOP    : default 192.168.8.168
Fan 2 / BOTTOM : default 192.168.8.191
Port           : 22333

Image mapping:
  TOP fan receives    0%  -> 60% of capture height
  BOTTOM fan receives 40% -> 100% of capture height
  Middle 40%-60% is the 20% overlap region.

Install dependencies:
  pip install customtkinter numpy mss opencv-python requests
"""

import socket
import time
from threading import Thread

import customtkinter as ctk
import cv2
import numpy as np
import requests
from mss import mss

# ----------------------------------------------------------------------------
# Theme — mirrors the web dashboard's "persistence of vision" palette
# ----------------------------------------------------------------------------
VOID = "#05060c"
VOID_2 = "#0a0d18"
SURFACE = "#10131f"
SURFACE_SOFT = "#161a29"
LINE = "#23273a"
INK = "#eef0fb"
INK_DIM = "#8a8fac"
CYAN = "#2fe6d8"
CYAN_HOVER = "#27c6ba"
MAGENTA = "#ff4fb4"
DANGER = "#ff5c72"
DANGER_HOVER = "#e0455f"
SUCCESS = "#34dd92"

ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("dark-blue")

# ----------------------------------------------------------------------------
# Streaming engine (unchanged logic from the original script)
# ----------------------------------------------------------------------------
NUMPIXELS = 100
Div = 320
TCP_PORT = 22333

DEFAULT_TOP_IP = "192.168.8.168"
DEFAULT_BOTTOM_IP = "192.168.8.191"

DEFAULT_BRIGHTNESS = 10
DEFAULT_CENTER = 10
DEFAULT_FPS = 12
OVERLAP_PERCENT = 20

running = False
sender_thread = None

sock_top = None
sock_bottom = None
capture_mode = "Screen"

posX = 0
posY = 0
posX2 = 360
posY2 = 600

last_time = time.time()

# UI widget handles, wired up once the App is constructed.
top_ip_entry = None
bottom_ip_entry = None
mode_combo = None
brightness_slider = None
center_slider = None
fps_slider = None
status_label = None
start_btn = None
stop_btn = None
fps_value_label = None
brightness_value_label = None
center_value_label = None


def set_status(msg: str, kind: str = "info") -> None:
    """Update the status pill text/color and echo to stdout."""
    print(msg)
    try:
        color = {"info": INK_DIM, "ok": SUCCESS, "error": DANGER}.get(kind, INK_DIM)
        status_label.configure(text=msg, text_color=color)
    except Exception:
        pass


def get_slider_int(slider, default_value, min_value, max_value):
    try:
        value = int(round(slider.get()))
        return max(min_value, min(max_value, value))
    except Exception:
        return default_value


def fetch_esp32_config(ip: str, label: str = "ESP32"):
    global NUMPIXELS, Div
    try:
        resp = requests.get(f"http://{ip}/status", timeout=1.5)
        if resp.status_code == 200:
            data = resp.json()
            NUMPIXELS = int(data.get("pixel_count", NUMPIXELS))
            Div = int(data.get("div", Div))
            print(f"{label} status OK: IP={ip}, NUMPIXELS={NUMPIXELS}, DIV={Div}")
            return True
        print(f"{label} status HTTP {resp.status_code}; using defaults")
    except Exception as e:
        print(f"{label} status not available: {e}. Continuing with defaults.")
    return False


def connect_socket(ip: str, label: str):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(5)
    s.connect((ip, TCP_PORT))
    s.settimeout(None)
    print(f"{label} connected: {ip}:{TCP_PORT}")
    return s


def frame_to_pov_jpeg(frame_bgr):
    if isinstance(frame_bgr, np.ndarray):
        img_np = frame_bgr
    else:
        img_np = np.array(frame_bgr)

    # mss gives BGRA; OpenCV webcam gives BGR
    if img_np.ndim == 3 and img_np.shape[2] == 4:
        img_np = cv2.cvtColor(img_np, cv2.COLOR_BGRA2BGR)

    target_size = NUMPIXELS * 2 - 1
    img_reduced = cv2.resize(img_np, (target_size, target_size), interpolation=cv2.INTER_AREA)
    img_reduced = cv2.rotate(img_reduced, cv2.ROTATE_90_CLOCKWISE)

    polar_image = cv2.warpPolar(
        img_reduced,
        (NUMPIXELS, Div),
        (img_reduced.shape[1] / 2, img_reduced.shape[0] / 2),
        min(img_reduced.shape[0], img_reduced.shape[1]) / 2,
        0,
    )

    bright = get_slider_int(brightness_slider, DEFAULT_BRIGHTNESS, 0, 100)
    center = get_slider_int(center_slider, DEFAULT_CENTER, 0, 100)

    # radial brightness compensation
    for i in range(NUMPIXELS):
        scale = ((100 - center) / NUMPIXELS * i + center) / 100 * bright / 100
        polar_image[:, i, 0] = np.clip(polar_image[:, i, 0] * scale, 0, 255)
        polar_image[:, i, 1] = np.clip(polar_image[:, i, 1] * scale, 0, 255)
        polar_image[:, i, 2] = np.clip(polar_image[:, i, 2] * scale, 0, 255)

    ok, encoded = cv2.imencode(".jpg", polar_image, [int(cv2.IMWRITE_JPEG_QUALITY), 60])
    if not ok:
        raise RuntimeError("JPEG encode failed")

    return encoded.tobytes()


def send_jpeg(sock, frame_data: bytes):
    header = str(len(frame_data)).ljust(5) + "\r"
    sock.sendall(header.encode("utf-8"))
    sock.sendall(frame_data)


def split_vertical_overlap(frame_np):
    """
    20% overlap:
      Top fan gets 0% -> 60%
      Bottom fan gets 40% -> 100%
    """
    h = frame_np.shape[0]
    overlap = OVERLAP_PERCENT / 100.0

    top_end = int(h * (0.5 + overlap / 2.0))  # 60% for 20% overlap
    bottom_start = int(h * (0.5 - overlap / 2.0))  # 40% for 20% overlap

    top_frame = frame_np[:top_end, :, :]
    bottom_frame = frame_np[bottom_start:, :, :]

    return top_frame, bottom_frame


def capture_loop():
    global running, sock_top, sock_bottom, last_time

    cap = None
    screen = None

    try:
        if capture_mode == "Webcam":
            cap = cv2.VideoCapture(0)
            if not cap.isOpened():
                set_status("Webcam open failed", "error")
                running = False
                return
        else:
            screen = mss()

        while running:
            start = time.time()

            if capture_mode == "Webcam":
                ret, frame = cap.read()
                if not ret:
                    time.sleep(0.02)
                    continue
                frame = cv2.flip(frame, 1)
            else:
                left = min(posX, posX2)
                top = min(posY, posY2)
                width = max(40, abs(posX2 - posX))
                height = max(80, abs(posY2 - posY))
                frame = np.array(screen.grab({"top": top, "left": left, "width": width, "height": height}))

            try:
                top_frame, bottom_frame = split_vertical_overlap(frame)

                top_data = frame_to_pov_jpeg(top_frame)
                bottom_data = frame_to_pov_jpeg(bottom_frame)

                send_jpeg(sock_top, top_data)
                send_jpeg(sock_bottom, bottom_data)

            except Exception as e:
                set_status(f"Connect/stream failed: {e}", "error")
                running = False
                break

            now = time.time()
            fps_now = int(1 / max(0.001, now - last_time))
            last_time = now
            print(f"FPS: {fps_now}")

            target_fps = get_slider_int(fps_slider, DEFAULT_FPS, 1, 25)
            frame_time = 1.0 / target_fps
            elapsed = time.time() - start
            time.sleep(max(0.001, frame_time - elapsed))

    finally:
        if cap is not None:
            cap.release()
        try:
            if sock_top is not None:
                sock_top.close()
        except Exception:
            pass
        try:
            if sock_bottom is not None:
                sock_bottom.close()
        except Exception:
            pass
        sock_top = None
        sock_bottom = None
        set_status("Stopped", "info")
        _set_button_states(running_now=False)


def _set_button_states(running_now: bool):
    try:
        if running_now:
            start_btn.configure(state="disabled")
            stop_btn.configure(state="normal")
        else:
            start_btn.configure(state="normal")
            stop_btn.configure(state="disabled")
    except Exception:
        pass


def startCapture():
    global running, capture_mode, sock_top, sock_bottom, sender_thread

    if running:
        set_status("Already streaming", "info")
        return

    top_ip = top_ip_entry.get().strip() or DEFAULT_TOP_IP
    bottom_ip = bottom_ip_entry.get().strip() or DEFAULT_BOTTOM_IP

    set_status("Connecting…", "info")
    fetch_esp32_config(top_ip, "TOP")
    fetch_esp32_config(bottom_ip, "BOTTOM")

    mode_text = mode_combo.get()
    capture_mode = "Webcam" if "Webcam" in mode_text else "Screen"

    try:
        sock_top = connect_socket(top_ip, "TOP fan")
    except Exception as e:
        set_status(f"TOP fan connection failed: {e}", "error")
        running = False
        return

    try:
        sock_bottom = connect_socket(bottom_ip, "BOTTOM fan")
    except Exception as e:
        try:
            sock_top.close()
        except Exception:
            pass
        sock_top = None
        set_status(f"BOTTOM fan connection failed: {e}", "error")
        running = False
        return

    running = True
    _set_button_states(running_now=True)
    sender_thread = Thread(target=capture_loop, daemon=True)
    sender_thread.start()
    set_status("Streaming · dual fan · 20% overlap", "ok")


def stopCapture():
    global running, sock_top, sock_bottom
    running = False
    try:
        if sock_top is not None:
            sock_top.close()
    except Exception:
        pass
    try:
        if sock_bottom is not None:
            sock_bottom.close()
    except Exception:
        pass
    set_status("Stopping…", "info")
    _set_button_states(running_now=False)


# ----------------------------------------------------------------------------
# Floating capture-area overlay (plain Tk — needs real window transparency,
# which CustomTkinter's CTkToplevel doesn't expose any better than Toplevel)
# ----------------------------------------------------------------------------
import tkinter as tk  # noqa: E402  (kept local to this section intentionally)
from tkinter import ttk  # noqa: E402


class ResizingCanvas(tk.Canvas):
    def __init__(self, parent, **kwargs):
        super().__init__(parent, **kwargs)
        self.bind("<Configure>", self.on_resize)
        self.height = self.winfo_reqheight()
        self.width = self.winfo_reqwidth()

    def on_resize(self, event):
        if self.width <= 0 or self.height <= 0:
            return
        wscale = event.width / self.width
        hscale = event.height / self.height
        self.width = event.width
        self.height = event.height
        self.scale("all", 0, 0, wscale, hscale)


class CaptureArea(tk.Toplevel):
    """Draggable / resizable on-screen frame showing what will be captured."""

    def __init__(self, parent):
        super().__init__(parent)
        self.overrideredirect(True)

        try:
            self.attributes("-alpha", 0.35)
        except Exception:
            pass
        try:
            self.attributes("-topmost", True)
        except Exception:
            pass

        bg = "#160019"
        try:
            self.attributes("-transparentcolor", "purple")
            bg = "purple"
        except Exception:
            bg = VOID

        self.geometry("360x600+500+220")
        self.canvas = ResizingCanvas(
            self,
            bg=bg,
            height=600,
            width=360,
            highlightthickness=2,
            highlightbackground=CYAN,
        )
        self.canvas.pack(fill=tk.BOTH, expand=tk.YES)

        # A thin midline marker showing the 20% overlap band between fans.
        self.canvas.create_rectangle(0, 0, 360, 600, outline=CYAN, width=2)
        self.canvas.create_line(0, 240, 360, 240, fill=MAGENTA, dash=(4, 3))
        self.canvas.create_line(0, 360, 360, 360, fill=MAGENTA, dash=(4, 3))

        self.label = tk.Label(
            self,
            text="CAPTURE AREA\nTOP → Fan 1   ·   BOTTOM → Fan 2\nDashed band = 20% overlap",
            bg=bg,
            fg=INK,
            font=("Segoe UI", 9, "bold"),
            justify="center",
        )
        self.label.place(relx=0.5, y=30, anchor="center")

        self.move = tk.Label(self, text="↕", bg=CYAN, fg=VOID, font=("Segoe UI", 10, "bold"))
        self.move.place(x=0, y=0, height=20, width=20)
        self.move.bind("<ButtonPress-1>", self.start_move)
        self.move.bind("<ButtonRelease-1>", self.stop_move)
        self.move.bind("<B1-Motion>", self.do_move)

        self.grip = ttk.Sizegrip(self)
        self.grip.place(height=18, width=18, relx=1.0, rely=1.0, anchor="se")
        self.grip.bind("<B1-Motion>", self.on_resize_drag)

        self.update_capture_coords()

    def update_capture_coords(self):
        global posX, posY, posX2, posY2
        posX = self.winfo_rootx() + 8
        posY = self.winfo_rooty() + 8
        posX2 = self.winfo_rootx() + self.winfo_width() - 8
        posY2 = self.winfo_rooty() + self.winfo_height() - 8

    def on_resize_drag(self, event):
        x0 = self.winfo_rootx()
        y0 = self.winfo_rooty()
        x1 = self.winfo_pointerx()
        y1 = self.winfo_pointery()
        self.geometry(f"{max(80, x1 - x0)}x{max(120, y1 - y0)}")
        self.update_capture_coords()

    def start_move(self, event):
        self._drag_x = event.x
        self._drag_y = event.y

    def stop_move(self, event):
        self._drag_x = None
        self._drag_y = None
        self.update_capture_coords()

    def do_move(self, event):
        dx = event.x - self._drag_x
        dy = event.y - self._drag_y
        x = self.winfo_x() + dx
        y = self.winfo_y() + dy
        self.geometry(f"+{x}+{y}")
        self.update_capture_coords()


# ----------------------------------------------------------------------------
# Main application window (CustomTkinter)
# ----------------------------------------------------------------------------
class App(ctk.CTk):
    def __init__(self):
        super().__init__()

        self.title("Fusion 5 · Dual Vertical Sender")
        self.geometry("380x760")
        self.minsize(380, 760)
        self.configure(fg_color=VOID)
        self.resizable(False, False)

        self.floater = CaptureArea(self)

        global top_ip_entry, bottom_ip_entry, mode_combo
        global brightness_slider, center_slider, fps_slider, status_label
        global start_btn, stop_btn
        global fps_value_label, brightness_value_label, center_value_label

        # ---- Header ----
        header = ctk.CTkFrame(self, fg_color="transparent")
        header.pack(fill="x", padx=22, pady=(22, 4))

        mark = ctk.CTkLabel(
            header, text="◎", width=34, height=34, font=("Segoe UI", 20),
            text_color=CYAN, fg_color=SURFACE, corner_radius=17,
        )
        mark.pack(side="left", padx=(0, 12))

        title_box = ctk.CTkFrame(header, fg_color="transparent")
        title_box.pack(side="left", fill="x", expand=True)
        ctk.CTkLabel(
            title_box, text="Fusion 5 Sender", font=("Segoe UI Semibold", 18), text_color=INK,
        ).pack(anchor="w")
        ctk.CTkLabel(
            title_box, text="Dual-fan vertical split · 20% overlap",
            font=("Segoe UI", 11), text_color=INK_DIM,
        ).pack(anchor="w")

        # ---- Status pill ----
        status_label = ctk.CTkLabel(
            self,
            text=f"Top={DEFAULT_TOP_IP} · Bottom={DEFAULT_BOTTOM_IP}",
            font=("Segoe UI", 11.5),
            text_color=INK_DIM,
            fg_color=SURFACE,
            corner_radius=10,
            height=32,
        )
        status_label.pack(fill="x", padx=22, pady=(10, 16))

        # ---- Card: fan IPs ----
        card1 = self._card("Fan addresses")
        ip_row = ctk.CTkFrame(card1, fg_color="transparent")
        ip_row.pack(fill="x", pady=(2, 0))
        ip_row.grid_columnconfigure(0, weight=1)
        ip_row.grid_columnconfigure(1, weight=1)

        top_box = ctk.CTkFrame(ip_row, fg_color="transparent")
        top_box.grid(row=0, column=0, sticky="ew", padx=(0, 6))
        ctk.CTkLabel(top_box, text="TOP FAN", font=("Segoe UI", 10), text_color=INK_DIM).pack(anchor="w")
        top_ip_entry = ctk.CTkEntry(
            top_box, justify="center", fg_color=SURFACE_SOFT, border_color=LINE,
            text_color=INK, corner_radius=9,
        )
        top_ip_entry.insert(0, DEFAULT_TOP_IP)
        top_ip_entry.pack(fill="x", pady=(4, 0))

        bottom_box = ctk.CTkFrame(ip_row, fg_color="transparent")
        bottom_box.grid(row=0, column=1, sticky="ew", padx=(6, 0))
        ctk.CTkLabel(bottom_box, text="BOTTOM FAN", font=("Segoe UI", 10), text_color=INK_DIM).pack(anchor="w")
        bottom_ip_entry = ctk.CTkEntry(
            bottom_box, justify="center", fg_color=SURFACE_SOFT, border_color=LINE,
            text_color=INK, corner_radius=9,
        )
        bottom_ip_entry.insert(0, DEFAULT_BOTTOM_IP)
        bottom_ip_entry.pack(fill="x", pady=(4, 0))

        # ---- Card: capture mode ----
        card2 = self._card("Capture source")
        mode_combo = ctk.CTkComboBox(
            card2, values=["Screen", "Webcam"], state="readonly",
            fg_color=SURFACE_SOFT, border_color=LINE, button_color=CYAN,
            button_hover_color=CYAN_HOVER, dropdown_fg_color=SURFACE,
            text_color=INK,
        )
        mode_combo.set("Screen")
        mode_combo.pack(fill="x", pady=(2, 0))

        # ---- Card: tuning sliders ----
        card3 = self._card("Stream tuning")

        brightness_slider, brightness_value_label = self._slider_row(
            card3, "Brightness", DEFAULT_BRIGHTNESS, 0, 100, "%"
        )
        center_slider, center_value_label = self._slider_row(
            card3, "Center brightness", DEFAULT_CENTER, 0, 100, "%"
        )
        fps_slider, fps_value_label = self._slider_row(
            card3, "Target FPS", DEFAULT_FPS, 1, 25, ""
        )

        # ---- Start / Stop ----
        btn_row = ctk.CTkFrame(self, fg_color="transparent")
        btn_row.pack(fill="x", padx=22, pady=(14, 22))
        btn_row.grid_columnconfigure(0, weight=1)
        btn_row.grid_columnconfigure(1, weight=1)

        start_btn = ctk.CTkButton(
            btn_row, text="Start streaming", command=startCapture,
            fg_color=CYAN, hover_color=CYAN_HOVER, text_color=VOID,
            font=("Segoe UI Semibold", 13), corner_radius=10, height=38,
        )
        start_btn.grid(row=0, column=0, sticky="ew", padx=(0, 6))

        stop_btn = ctk.CTkButton(
            btn_row, text="Stop", command=stopCapture,
            fg_color=SURFACE_SOFT, hover_color=DANGER_HOVER, text_color=INK,
            font=("Segoe UI Semibold", 13), corner_radius=10, height=38,
            state="disabled",
        )
        stop_btn.grid(row=0, column=1, sticky="ew", padx=(6, 0))

        self.protocol("WM_DELETE_WINDOW", self.on_close)

    def _card(self, title):
        card = ctk.CTkFrame(self, fg_color=SURFACE, corner_radius=14)
        card.pack(fill="x", padx=22, pady=(0, 14))
        ctk.CTkLabel(
            card, text=title.upper(), font=("Segoe UI", 10.5, "bold"),
            text_color=CYAN,
        ).pack(anchor="w", padx=16, pady=(14, 8))
        inner = ctk.CTkFrame(card, fg_color="transparent")
        inner.pack(fill="x", padx=16, pady=(0, 16))
        return inner

    def _slider_row(self, parent, label, default, lo, hi, suffix):
        row = ctk.CTkFrame(parent, fg_color="transparent")
        row.pack(fill="x", pady=(0, 10))

        top = ctk.CTkFrame(row, fg_color="transparent")
        top.pack(fill="x")
        ctk.CTkLabel(top, text=label, font=("Segoe UI", 11.5), text_color=INK).pack(side="left")
        value_label = ctk.CTkLabel(
            top, text=f"{default}{suffix}", font=("Segoe UI", 11.5, "bold"), text_color=CYAN,
        )
        value_label.pack(side="right")

        slider = ctk.CTkSlider(
            row, from_=lo, to=hi, number_of_steps=(hi - lo),
            progress_color=CYAN, button_color=CYAN, button_hover_color=CYAN_HOVER,
            fg_color=LINE,
            command=lambda v: value_label.configure(text=f"{int(round(v))}{suffix}"),
        )
        slider.set(default)
        slider.pack(fill="x", pady=(6, 0))
        return slider, value_label

    def on_close(self):
        stopCapture()
        try:
            self.floater.destroy()
        except Exception:
            pass
        self.destroy()


if __name__ == "__main__":
    app = App()
    app.mainloop()
