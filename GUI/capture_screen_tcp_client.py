import socket
import time
import cv2
import os
import numpy as np
from PIL import Image, ImageGrab

def resize_with_padding(img, target_size=(320, 240), background_color=(0, 0, 0)):
    # 计算目标宽高比和原始宽高比
    target_ratio = target_size[0] / target_size[1]
    original_ratio = img.width / img.height
    
    # 根据宽高比决定如何缩放图像
    if original_ratio > target_ratio:
        # 原始图像比目标图像宽，因此以宽度为基准进行缩放
        resized_width = target_size[0]
        resized_height = round(resized_width / original_ratio)
    else:
        # 原始图像比目标图像窄，因此以高度为基准进行缩放
        resized_height = target_size[1]
        resized_width = round(resized_height * original_ratio)

    # 缩放图像
    img = img.resize((resized_width, resized_height), Image.Resampling.LANCZOS)

    # 创建一个新的图像，背景颜色为黑色
    new_img = Image.new("RGB", target_size, background_color)
    
    # 计算粘贴的位置
    top_left_x = (target_size[0] - resized_width) // 2
    top_left_y = (target_size[1] - resized_height) // 2

    # 将缩放后的图像粘贴到黑色背景中心
    new_img.paste(img, (top_left_x, top_left_y))

    return new_img

def capture_screen():
    # 使用Pillow的ImageGrab抓取屏幕
    screen = ImageGrab.grab()
    # 调整大小并添加黑色填充
    screen_resized = resize_with_padding(screen, (320, 240))
    screen_resized = np.array(screen_resized)
    screen_resized = cv2.cvtColor(screen_resized, cv2.COLOR_RGB2BGR)
    return screen_resized

def send_image(tcp_client, quality=50):
    screen_resized = capture_screen()
    encode_param = [int(cv2.IMWRITE_JPEG_QUALITY), quality]
    result, encoded_image = cv2.imencode('.jpg', screen_resized, encode_param)
    image_data = encoded_image.tobytes()
    image_size = len(image_data) + 3
    tcp_client.send((image_size & 0xff).to_bytes(1, 'little'))
    tcp_client.send(((image_size >> 8) & 0xff).to_bytes(1, 'little'))
    image_data = image_data + b'\xAA\xBB\xCC'
    tcp_client.sendall(image_data)

def start_client(server_ip, port):
    tcp_client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        tcp_client.connect((server_ip, port))
        print(f"Connected to {server_ip}:{port}")
        send_image(tcp_client)
        while True:
            t0 = time.time()
            response = tcp_client.recv(2)
            if response == b'ok':
                send_image(tcp_client)
            else:
                print("Unrecognized response, stopping...")
                break
            t1 = time.time()
            # fps = 1 / (t1 - t0)
            # print(f"FPS: {fps:.2f}")
    except Exception as e:
        print(f"An error occurred: {e}")
        tcp_client.close()
        return 0

import threading

# Function to handle individual server connection
def handle_connection(server_ip, port):
    while start_client(server_ip, port) == 0:
        print(f"Reconnecting to {server_ip}...")
        time.sleep(1)

def main():
    server_ips = ['192.168.31.171', '192.168.31.184', '192.168.31.26']
    port = 40111
    threads = []

    # Start a thread for each server IP
    for ip in server_ips:
        thread = threading.Thread(target=handle_connection, args=(ip, port))
        thread.start()
        threads.append(thread)

    # Wait for all threads to finish
    for thread in threads:
        thread.join()

if __name__ == '__main__':
    main()