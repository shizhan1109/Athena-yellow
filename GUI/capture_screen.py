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

def send_image(tcp_client, quality=70):
    # 读取JPEG文件内容
    #image = cv2.imread(image_path, cv2.IMREAD_COLOR)

    screen_resized = capture_screen()
    
    # 设置JPEG压缩质量 (从0到100，高质量值意味着更好的质量，但文件更大)
    encode_param = [int(cv2.IMWRITE_JPEG_QUALITY), quality]  # 将压缩质量设置为50%
    
    
    # 编码图片
    result, encoded_image = cv2.imencode('.jpg', screen_resized, encode_param)

    # 检查编码是否成功
    if not result:
        print('Error encoding image')
        return

    # 将图片数据转换为字节流
    image_data = encoded_image.tobytes()
    
    # 发送图片大小信息
    image_size = len(image_data) + 3
    tcp_client.send((image_size & 0xff).to_bytes(1, 'little')) # 低8位
    tcp_client.send(((image_size >> 8) & 0xff).to_bytes(1, 'little')) # 9-16位
    
    # 发送图片数据
    image_data = image_data + b'\xAA\xBB\xCC'
    tcp_client.sendall(image_data)

class ImageServer:
    def __init__(self, server_ip, port):
        self.server_ip = server_ip
        self.port = port
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    def start(self, quality=70):
        self.server_socket.bind((self.server_ip, self.port))
        self.server_socket.listen(1)
        print(f"Listening on {self.server_ip}:{self.port}")

        # 等待设备连接
        client_socket, addr = self.server_socket.accept()
        print(f"Connected by {addr}")

        try:
            # 循环发送图片数据
            while True:
                t0 = time.time()
                response = client_socket.recv(2)
                if response == b'ok':
                    send_image(client_socket, quality)
                else:
                    print("Unrecognized response, stopping...")
                    break
                t1 = time.time()
                fps = 1 / (t1 - t0)
                print(f"FPS: {fps:.2f}")
        except Exception as e:
            print(f"An error occurred: {e}")
        finally:
            client_socket.close()
            self.server_socket.close()


def main():
    # 服务器IP和端口配置
    server_ip = '192.168.31.86'
    port = 40111

    # 创建ImageServer实例
    server = ImageServer(server_ip, port)
    server.start(quality=70)

if __name__ == '__main__':
    main()
