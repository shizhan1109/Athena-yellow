# coding:utf-8
import sys
from PySide6.QtCore import Qt, QTimer, QRectF, Slot
from PySide6.QtGui import QIcon, QPixmap, QPainter, QImage, QBrush, QColor, QFont, QPainterPath
from PySide6.QtWidgets import QApplication, QFrame, QStackedWidget, QHBoxLayout, QLabel, QVBoxLayout, QGridLayout

from qfluentwidgets import (NavigationInterface, NavigationItemPosition, TeachingTip, InfoBarIcon, TeachingTipTailPosition,
                            isDarkTheme, setTheme, Theme, setThemeColor,
                            PrimaryPushButton, ComboBox, LineEdit)
from qfluentwidgets import FluentIcon as FIF
from qframelesswindow import FramelessWindow, StandardTitleBar

from PIL import ImageGrab
import numpy as np
import time
import serial
import serial.tools.list_ports

from zeroconf import ServiceBrowser, Zeroconf

from capture_screen_tcp_client_qt import ScreenCaptureThread

global_devices = []
is_screen = False

class MyListener:
    def __init__(self):
        self.devices = []

    def remove_service(self, zeroconf, type, name):
        print(f"Service {name} removed")

    def add_service(self, zeroconf, type, name):
        info = zeroconf.get_service_info(type, name)
        if info:
            addresses = ", ".join(str(addr) for addr in info.parsed_addresses())
            # print(f"Service {name} added, IP addresses: {addresses}, Port: {info.port}")
            name = name.split(".")[0]
            self.devices.append({"name": name, "addresses": addresses, "port": info.port})

    def get_devices(self):
        return self.devices
    
    def update_service(self, zeroconf, type, name):
        print(f"Service {name} updated")

class RoundedLabel(QLabel):
    def __init__(self, text, parent=None):
        super().__init__(text, parent)
        self.setAlignment(Qt.AlignCenter)  # 居中文本
        self.setFont(QFont('Arial', 10, 5))  # 设置字体加粗

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)  # 开启抗锯齿

        path = QPainterPath()
        path.addRoundedRect(QRectF(self.rect()), 12, 12)  # 绘制圆角矩形，角半径为10

        # 设置填充颜色
        if len(self.text()) < 7:
            painter.setBrush(QColor(180, 20, 20))
        else:
            painter.setBrush(QBrush(Qt.green))
        painter.drawPath(path)

        painter.setPen(Qt.black)  # 设置边框颜色
        painter.drawPath(path)

        super().paintEvent(event)  # 调用基类的paintEvent继续绘制文本

class DisplayWidget(QFrame):
    def __init__(self, title: str, parent=None):
        super().__init__(parent=parent)
        # self.setStyleSheet("QWidget{ border: 1px solid black; }")
        self.hBoxLayout = QHBoxLayout(self)
        self.hBoxLayout.setContentsMargins(10, 10, 10, 10)
        self.hBoxLayout.setSpacing(5)

        # 创建一个垂直布局
        self.vbox = QVBoxLayout()
        self.vbox.setContentsMargins(0, 0, 0, 0)
        self.vbox.setSpacing(5)
        self.hBoxLayout.addLayout(self.vbox)

        # 创建一个 QLabel 用于显示标题
        self.title_label = QLabel(title, self)
        # 设置字体大小
        self.title_label.setFont(QFont('Arial', 20))
        # 设置对齐方式
        self.title_label.setAlignment((Qt.AlignLeft | Qt.AlignTop))
        self.title_label.setFixedHeight(40)
        self.vbox.addWidget(self.title_label)

        # 创建一个 QLabel 用于显示屏幕截图
        self.screen_label = QLabel(self)
        self.screen_label.setAlignment((Qt.AlignLeft | Qt.AlignTop))
        self.vbox.addWidget(self.screen_label)

        # 创建一个按钮用于开始/停止屏幕截图
        self.toggleButton = PrimaryPushButton(FIF.LINK, 'Start screen', self)
        self.toggleButton.setFixedSize(200, 40)
        self.toggleButton.clicked.connect(self.start_stop_screen)
        self.vbox.addWidget(self.toggleButton, 0, (Qt.AlignRight | Qt.AlignTop))

        # 创建第二个垂直布局
        self.vbox2 = QVBoxLayout()
        self.vbox2.setContentsMargins(0, 0, 0, 0)
        self.vbox2.setSpacing(5)
        self.hBoxLayout.addLayout(self.vbox2)

        self.display_node_info()

        self.setLayout(self.hBoxLayout)

        # 定时器来周期性更新屏幕图像
        self.timer = QTimer(self)
        self.timer.timeout.connect(lambda: self.update_image() or self.update_node_info())
        self.timer.start(100)

        self.screen_capture_thread = None

    def update_image(self):
        # 获取当前屏幕截图
        screen = ImageGrab.grab()
        screen = screen.resize((screen.width // 4, screen.height // 4))
        screen_array = np.array(screen)

        # 转换为 QImage，确保格式正确
        screen_image = QImage(screen_array, screen_array.shape[1], screen_array.shape[0], QImage.Format_RGB888)
        screen_pixmap = QPixmap.fromImage(screen_image)
        self.screen_label.setPixmap(screen_pixmap)
    
    def start_stop_screen(self):
        # 切换按钮文本
        global global_devices
        global is_screen

        if self.toggleButton.text() == 'Start screen':
            is_screen = True
            self.toggleButton.setText('Stop screen')
            TeachingTip.create(
                target=self.toggleButton,
                icon=InfoBarIcon.SUCCESS,
                title='Success',
                content='Screen capture started.',
                isClosable=True,
                tailPosition=TeachingTipTailPosition.TOP,
                duration=2000,
                parent=self
            )

            # Screen capture start
            print(global_devices)
            self.capture_screen()
        else:
            is_screen = False
            self.toggleButton.setText('Start screen')
            TeachingTip.create(
                target=self.toggleButton,
                icon=InfoBarIcon.SUCCESS,
                title='Success',
                content='Screen capture stopped.',
                isClosable=True,
                tailPosition=TeachingTipTailPosition.TOP,
                duration=2000,
                parent=self
            )

            # Screen capture stop
            print(global_devices)
            self.stop_capturing()

    def display_node_info(self):
        # 创建一个 QLabel 用于显示标题
        self.node_title = QLabel("Node:", self)
        # 设置字体大小
        self.node_title.setFont(QFont('Arial', 20))
        # 设置对齐方式
        self.node_title.setAlignment((Qt.AlignLeft | Qt.AlignTop))
        self.node_title.setFixedHeight(40)
        self.vbox2.addWidget(self.node_title)

        # 创建9个 QLabel 用于显示节点信息
        node_names = ['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I']
        text_list = ["offline", "offline", "offline", "offline", "offline", "offline", "offline", "offline", "offline"]

        self.node_labels = []
        for index, name in enumerate(node_names):
            node_label = QLabel(f'Node {name}: {text_list[index]}', self)
            node_label.setAlignment(Qt.AlignLeft | Qt.AlignTop)
            self.vbox2.addWidget(node_label)
            self.node_labels.append(node_label)

        # 弹簧
        self.vbox2.addStretch(1)
    
    def update_node_info(self):
        global global_devices
        global is_screen

        node_names = ['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I']
        text_list = ["offline", "offline", "offline", "offline", "offline", "offline", "offline", "offline", "offline"]

        for i, device in enumerate(global_devices):
            if i >= len(node_names):
                break
            text_list[i] = "online"
        
        for label, text in zip(self.node_labels, text_list):
            label.setText(f"Node {node_names[self.node_labels.index(label)]}: {text}")
            # 如果是在线状态，设置字体颜色为绿色
            if text == "online":
                label.setStyleSheet("QLabel{color: green;}")
            else:
                label.setStyleSheet("QLabel{color: red;}")

    @Slot()
    def capture_screen(self):
        global global_devices
        servers = [
            {'name': '30C6F725C5AC', 'addresses': '192.168.31.182', 'port': 40111}, 
            {'name': '782184933B28', 'addresses': '192.168.31.168', 'port': 40111}, 
            {'name': 'M5AY-7821848DDCEC', 'addresses': '192.168.31.23', 'port': 40111}
        ]
        servers = global_devices
        self.screen_capture_thread = ScreenCaptureThread(servers)
        self.screen_capture_thread.connection_error.connect(self.handle_error)
        self.screen_capture_thread.start()

    @Slot()
    def stop_capturing(self):
        if self.screen_capture_thread is not None:
            self.screen_capture_thread.stop()
            self.screen_capture_thread.wait()
            self.screen_capture_thread = None

    @Slot(str)
    def handle_error(self, error_message):
        print(f"Connection Error: {error_message}")

class NetworkingWidget(QFrame):
    def __init__(self, text: str, parent=None):
        super().__init__(parent=parent)
        self.devices = []

        # self.setStyleSheet("QWidget{ border: 1px solid black; }")
        self.hBoxLayout = QHBoxLayout(self)

        # 创建一个垂直布局
        self.vbox = QVBoxLayout()
        self.hBoxLayout.addLayout(self.vbox)

        # title label
        self.title_label = QLabel(text, self)
        self.title_label.setFont(QFont('Arial', 20))
        self.title_label.setAlignment(Qt.AlignTop)
        self.vbox.addWidget(self.title_label, 1, Qt.AlignTop)
        self.setObjectName(text.replace(' ', '-'))

        # Bluetooth broadcast
        self.hBluetooth = QHBoxLayout()
        self.vbox.addLayout(self.hBluetooth)

        # create a combo box
        self.comboBox = ComboBox()
        self.comboBox.addItems([port.device for port in serial.tools.list_ports.comports()])
        self.comboBox.clicked.connect(lambda: self.comboBox.clear() or self.comboBox.addItems([port.device for port in serial.tools.list_ports.comports()]))
        self.comboBox.setFixedWidth(100)
        self.hBluetooth.addWidget(self.comboBox)

        # create a SSID line edit
        self.ssidLineEdit = LineEdit()
        self.ssidLineEdit.setPlaceholderText('SSID')
        self.ssidLineEdit.setFixedWidth(250)
        self.hBluetooth.addWidget(self.ssidLineEdit)

        # create a password line edit
        self.passwordLineEdit = LineEdit()
        self.passwordLineEdit.setPlaceholderText('Password')
        self.passwordLineEdit.setFixedWidth(250)
        self.hBluetooth.addWidget(self.passwordLineEdit)

        # create a push button
        self.broadcastButton = PrimaryPushButton(FIF.SEND, 'Broadcast')
        self.broadcastButton.setFixedWidth(200)
        self.broadcastButton.clicked.connect(lambda: self.send_broadcast())
        self.hBluetooth.addWidget(self.broadcastButton)

        # found devices label
        self.foundDevicesLabel = QLabel('Found devices:', self)
        self.foundDevicesLabel.setFont(QFont('Arial', 20))
        self.foundDevicesLabel.setAlignment(Qt.AlignTop)
        self.vbox.addWidget(self.foundDevicesLabel, 1, Qt.AlignTop)

        # create a 3*3 grid layout
        self.grid_layout()

        self.vbox.addStretch(99)

        #self.setLayout(self.hBoxLayout)

    def send_broadcast(self):
        port = self.comboBox.currentText()
        ssid = self.ssidLineEdit.text()
        password = self.passwordLineEdit.text()

        if ssid == '' or len(password) < 8:
            TeachingTip.create( 
                target=self.broadcastButton,
                icon=InfoBarIcon.WARNING,
                title='Warning',
                content='SSID cannot be empty or password length less than 8 characters.',
                isClosable=True,
                tailPosition=TeachingTipTailPosition.BOTTOM,
                duration=2000,
                parent=self
            )
            return
        elif port == '':
            TeachingTip.create( 
                target=self.broadcastButton,
                icon=InfoBarIcon.WARNING,
                title='Warning',
                content='Please select a port.',
                isClosable=True,
                tailPosition=TeachingTipTailPosition.BOTTOM,
                duration=2000,
                parent=self
            )
            return
        else:
            if self.send_wifi_config(port, ssid, password):
                print(f'send Port: {port}, SSID: {ssid}, Password: {password}')
                TeachingTip.create( 
                    target=self.broadcastButton,
                    icon=InfoBarIcon.SUCCESS,
                    title='Success',
                    content='Broadcasting...',
                    isClosable=True,
                    tailPosition=TeachingTipTailPosition.BOTTOM,
                    duration=2000,
                    parent=self
                )   
            else:
                print('Failed to broadcast wifi config.')

    def send_wifi_config(self, port, ssid, password):
        try:
            ser = serial.Serial(port, baudrate=115200, timeout=1)

            command = f"bwifi {ssid} {password}\n"
            ser.write(command.encode())
        except Exception as e:
            print(f'Error: {e}')
            return False
        finally:
            ser.close()
        return True

    def grid_layout(self, labels=[]):
        size = 110
        # 如果没有初始化布局，则初始化布局
        if not hasattr(self, 'hGrid'):
            self.hGrid = QHBoxLayout()
            self.vbox.addLayout(self.hGrid)

            self.grid = QGridLayout()
            self.grid.setSpacing(5)
            self.hGrid.addLayout(self.grid)

            # Button Layout
            self.hSTButton = QHBoxLayout()
            self.vbox.addLayout(self.hSTButton)
            self.hSTButton.addStretch(1)

            self.searchButton = PrimaryPushButton('Search')
            self.searchButton.setFixedWidth(165)
            self.searchButton.clicked.connect(lambda: self.update_devices())
            self.hSTButton.addWidget(self.searchButton)

            self.hSTButton.addStretch(1)

        # 清除现有的网格元素
        for i in reversed(range(self.grid.count())): 
            widget = self.grid.itemAt(i).widget()
            if widget is not None:
                widget.setParent(None)

        # 创建新的圆角矩形标签并设置固定大小
        if labels == []:
            labels = ['Node A', 'Node B', 'Node C', 'Node D', 'Node E', 'Node F', 'Node G', 'Node H', 'Node I']
        self.nodes = [RoundedLabel(label, self) for label in labels]
        for node in self.nodes:
            node.setFixedSize(size, size)

        # 将标签添加到网格布局中
        positions = [(i, j) for i in range(3) for j in range(3)]
        for node, position in zip(self.nodes, positions):
            self.grid.addWidget(node, *position)

    def update_devices(self):
        global is_screen
        if is_screen:
            TeachingTip.create( 
                target=self.searchButton,
                icon=InfoBarIcon.ERROR,
                title='Error',
                content='Please stop screen capture first.',
                isClosable=True,
                tailPosition=TeachingTipTailPosition.BOTTOM,
                duration=2000,
                parent=self
            )
            return

        self.searchButton.setText('Searching...')
        self.searchButton.setEnabled(False)
        TeachingTip.create( 
            target=self.searchButton,
            icon=InfoBarIcon.INFORMATION,
            title='Searching',
            content='Searching devices...',
            isClosable=True,
            tailPosition=TeachingTipTailPosition.BOTTOM,
            duration=2000,
            parent=self
        )
        # 使用 QTimer 延迟5s
        QTimer.singleShot(300, self.search_devices)  # 100毫秒后执行
    
    def search_devices(self):
        zeroconf = Zeroconf()
        listener = MyListener()
        browser = ServiceBrowser(zeroconf, "_ayServer._tcp.local.", listener)
        time.sleep(5)
        self.devices = listener.get_devices().copy()
        self.devices = sorted(self.devices, key=lambda x: x['name'])
        global global_devices
        global_devices = self.devices
        zeroconf.close()

        devices_str_list = ['Node A', 'Node B', 'Node C', 'Node D', 'Node E', 'Node F', 'Node G', 'Node H', 'Node I']
        for i, device in enumerate(self.devices):
            if i >= len(devices_str_list):
                break

            name = device['name'].replace('-', '\n')
            ip = device['addresses']
            port = device['port']
            device_str = f"{name}\n\n{ip}:\n{port}"
            devices_str_list[i] = device_str
        
        self.grid_layout(devices_str_list)
        self.searchButton.setText('Search')
        self.searchButton.setEnabled(True)
        print(self.devices)

class Window(FramelessWindow):

    def __init__(self):
        super().__init__()
        self.setTitleBar(StandardTitleBar(self))

        # change the theme color
        setThemeColor('#51247A')

        self.hBoxLayout = QHBoxLayout(self)
        self.navigationInterface = NavigationInterface(self, showMenuButton=True)
        self.stackWidget = QStackedWidget(self)

        # create sub interface
        self.displayInterface = DisplayWidget('Display Adapter', self)
        self.networkingInterface = NetworkingWidget('Networking', self)

        # initialize layout
        self.initLayout()

        # add items to navigation interface
        self.initNavigation()

        self.initWindow()

    def initLayout(self):
        self.hBoxLayout.setSpacing(0)
        self.hBoxLayout.setContentsMargins(0, self.titleBar.height(), 0, 0)
        self.hBoxLayout.addWidget(self.navigationInterface)
        self.hBoxLayout.addWidget(self.stackWidget)
        self.hBoxLayout.setStretchFactor(self.stackWidget, 1)

    def initNavigation(self):
        self.addSubInterface(self.displayInterface, FIF.LINK, 'Display Adapter')
        self.addSubInterface(self.networkingInterface, FIF.WIFI, 'Networking')

        self.navigationInterface.addSeparator()

        # add setThemeInterface to bottom
        self.navigationInterface.addItem(
            routeKey='Set Theme',
            icon=FIF.CONSTRACT,
            text='Set Theme',
            onClick=lambda: self.updateTheme(),
            position=NavigationItemPosition.BOTTOM,
            tooltip='Set Theme'
        )

        # set the maximum width
        self.navigationInterface.setExpandWidth(200)

        self.stackWidget.currentChanged.connect(self.onCurrentInterfaceChanged)
        self.stackWidget.setCurrentIndex(1)

    def initWindow(self):
        self.resize(900, 500)
        # self.setWindowIcon(QIcon('resource/logo.png'))
        self.setWindowTitle('Athena-Yellow')
        self.titleBar.setAttribute(Qt.WA_StyledBackground)

        desktop = QApplication.screens()[0].availableGeometry()
        w, h = desktop.width(), desktop.height()
        self.move(w//2 - self.width()//2, h//2 - self.height()//2)

        self.setQss()

    def addSubInterface(self, interface, icon, text: str, position=NavigationItemPosition.TOP, parent=None):
        """ add sub interface """
        self.stackWidget.addWidget(interface)
        self.navigationInterface.addItem(
            routeKey=interface.objectName(),
            icon=icon,
            text=text,
            onClick=lambda: self.switchTo(interface),
            position=position,
            tooltip=text,
            parentRouteKey=parent.objectName() if parent else None
        )

    def setQss(self):
        color = 'dark' if isDarkTheme() else 'light'
        with open(f'resource/{color}/demo.qss', encoding='utf-8') as f:
            self.setStyleSheet(f.read())

    def updateTheme(self):
        setTheme(Theme.LIGHT if isDarkTheme() else Theme.DARK)
        self.setQss()

    def switchTo(self, widget):
        self.stackWidget.setCurrentWidget(widget)

    def onCurrentInterfaceChanged(self, index):
        widget = self.stackWidget.widget(index)
        self.navigationInterface.setCurrentItem(widget.objectName())

if __name__ == '__main__':
    app = QApplication(sys.argv)
    w = Window()
    w.show()
    app.exec()
