import socket
import time
import cv2
import numpy as np
from PIL import Image, ImageGrab
from PySide6.QtCore import QThread, Signal, Slot
import threading

class ScreenCaptureThread(QThread):
    connection_error = Signal(str)

    def __init__(self, servers, parent=None):
        super(ScreenCaptureThread, self).__init__(parent)
        self.servers = servers

    def resize_with_padding(self, img, target_size=(960, 720), background_color=(0, 0, 0)):
        target_ratio = target_size[0] / target_size[1]
        original_ratio = img.width / img.height
        
        if original_ratio > target_ratio:
            resized_width = target_size[0]
            resized_height = round(resized_width / original_ratio)
        else:
            resized_height = target_size[1]
            resized_width = round(resized_height * original_ratio)

        img = img.resize((resized_width, resized_height), Image.Resampling.LANCZOS)
        new_img = Image.new("RGB", target_size, background_color)
        top_left_x = (target_size[0] - resized_width) // 2
        top_left_y = (target_size[1] - resized_height) // 2
        new_img.paste(img, (top_left_x, top_left_y))

        return new_img

    def capture_screen(self, part=None):
        screen = ImageGrab.grab()

        if part is None:
            screen_resized = self.resize_with_padding(screen, (320, 240))
        else:
            screen_resized = self.resize_with_padding(screen, (960, 720))

        screen_resized = np.array(screen_resized)
        screen_resized = cv2.cvtColor(screen_resized, cv2.COLOR_RGB2BGR)

        if part is not None and 0 <= part < 9:
            col = part % 3
            row = part // 3
            x_start = col * 320
            y_start = row * 240
            screen_resized = screen_resized[y_start:y_start+240, x_start:x_start+320]

        return screen_resized

    def send_image(self, tcp_client, quality=50, part=None):
        screen_resized = self.capture_screen(part)
        encode_param = [int(cv2.IMWRITE_JPEG_QUALITY), quality]
        result, encoded_image = cv2.imencode('.jpg', screen_resized, encode_param)
        image_data = encoded_image.tobytes()
        image_size = len(image_data) + 3
        tcp_client.send((image_size & 0xff).to_bytes(1, 'little'))
        tcp_client.send(((image_size >> 8) & 0xff).to_bytes(1, 'little'))
        image_data = image_data + b'\xAA\xBB\xCC'
        tcp_client.sendall(image_data)

    def start_client(self, server_ip, port, part=None):
        tcp_client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            tcp_client.connect((server_ip, port))
            print(f"Connected to {server_ip}:{port}")
            self.send_image(tcp_client, part=part)
            while not self.stop_event.is_set():
                response = tcp_client.recv(2)
                if response == b'ok' and not self.stop_event.is_set():
                    self.send_image(tcp_client, part=part)
                else:
                    print("Unrecognized response, stopping...")
                    break
        except Exception as e:
            print(f"An error occurred: {e}")
            self.connection_error.emit(f"Error: {e}")
            return 0
        finally:
            tcp_client.close()
        return 1

    def close_all_connections(self):
        print("Closing all connections...")
        self.stop_event.set()

    def run(self):
        self.stop_event = threading.Event()
        threads = []

        for index, server in enumerate(self.servers):
            ip = server['addresses']
            port = server['port']
            part = index
            thread = threading.Thread(target=self.handle_connection, args=(ip, port, part))
            thread.start()
            threads.append(thread)

        for thread in threads:
            thread.join()

    def handle_connection(self, server_ip, port, part):
        while not self.stop_event.is_set():
            result = self.start_client(server_ip, port, part)
            if result == 0 and not self.stop_event.is_set():
                time.sleep(0.5)
            else:
                break

    @Slot()
    def stop(self):
        self.close_all_connections()


# test
from PySide6.QtWidgets import QApplication, QPushButton, QVBoxLayout, QWidget
from PySide6.QtCore import Slot

class MainWindow(QWidget):
    def __init__(self):
        super(MainWindow, self).__init__()
        self.initUI()

    def initUI(self):
        self.setWindowTitle("Screen Capture Client")
        layout = QVBoxLayout()

        self.startButton = QPushButton("Start")
        self.stopButton = QPushButton("Stop")

        self.startButton.clicked.connect(self.start_capturing)
        self.stopButton.clicked.connect(self.stop_capturing)

        layout.addWidget(self.startButton)
        layout.addWidget(self.stopButton)

        self.setLayout(layout)
        self.screen_capture_thread = None

    @Slot()
    def start_capturing(self):
        servers = [
            {'name': '30C6F725C5AC', 'addresses': '192.168.31.182', 'port': 40111}, 
            {'name': '782184933B28', 'addresses': '192.168.31.168', 'port': 40111}, 
            {'name': 'M5AY-7821848DDCEC', 'addresses': '192.168.31.23', 'port': 40111}
        ]
        self.screen_capture_thread = ScreenCaptureThread(servers)
        self.screen_capture_thread.connection_error.connect(self.handle_error)
        self.screen_capture_thread.start()

    @Slot()
    def stop_capturing(self):
        if self.screen_capture_thread is not None:
            self.screen_capture_thread.stop()
            self.screen_capture_thread = None

    @Slot(str)
    def handle_error(self, error_message):
        print(f"Connection Error: {error_message}")

if __name__ == '__main__':
    app = QApplication([])
    window = MainWindow()
    window.show()
    app.exec()