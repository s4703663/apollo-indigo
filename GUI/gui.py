import sys
from PySide6.QtWidgets import (
    QApplication,
    QMainWindow,
    QLabel,
    QPushButton,
    QVBoxLayout,
    QWidget,
    QFileDialog,
    QLineEdit,
    QDialog,
    QDialogButtonBox,
    QComboBox,
    QHBoxLayout,
    QGridLayout,
    QSizePolicy,
)
from PySide6.QtGui import QPixmap, QImageReader, QMovie, QImage, QPainter, QPen
from PySide6.QtCore import Qt, QSize, QRect
from serial.tools import list_ports
import serial

class ImageSenderApp(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Image Sender")
        # Widgets
        self.com_port_label = QLabel("Select Port:")
        self.com_port_options = QComboBox()
        self.refresh_button = QPushButton("Refresh Ports")
        self.refresh_button.clicked.connect(self.refresh_ports)
        self.refresh_ports()  # Initial refresh
        self.connect_button = QPushButton("Connect")
        self.connect_button.clicked.connect(self.connect_device)
        self.choose_file_button = QPushButton("Choose File")
        self.choose_file_button.clicked.connect(self.choose_file)
        self.file_path_label = QLabel("Selected File: ")
        self.send_file_button = QPushButton("Send File")
        self.send_file_button.clicked.connect(self.send_file)
        # Image Display
        self.image_label = QLabel()
        self.image_label.setAlignment(Qt.AlignCenter)
        self.image_label.setFixedSize(900, 600)
        self.image_label.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
        # Layout
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        layout = QVBoxLayout(central_widget)
        layout.addWidget(self.com_port_label)
        layout.addWidget(self.com_port_options)
        layout.addWidget(self.refresh_button)
        layout.addWidget(self.connect_button)
        layout.addWidget(self.choose_file_button)
        layout.addWidget(self.file_path_label)
        layout.addWidget(self.send_file_button)
        layout.addWidget(self.image_label)
        # Variables
        self.serial_conn = None
        self.file_path = None
        self.gif_movie = None

    def refresh_ports(self):
        '''Refresh the list of ports available in the combo box'''
        ports = list(serial.tools.list_ports.comports())
        ports = [port.device for port in ports]
        self.com_port_options.clear()
        self.com_port_options.addItems(ports)

    def connect_device(self):
        self.com_port = self.com_port_options.currentText()
        try:
            self.serial_conn = serial.Serial(self.com_port, baudrate=9600, timeout=1)
            print(f"Connected to {self.com_port}")
        except serial.SerialException as e:
            print(f"Error connecting to {self.com_port}: {e}")
        except Exception as ex:
            print(f"An unexpected error occurred: {ex}")

    def choose_file(self):
        file_dialog = QFileDialog()
        file_dialog.setNameFilter("Files (*.png *.jpg *.jpeg *.gif *.mp4)")
        if file_dialog.exec():
            self.file_path = file_dialog.selectedFiles()[0]
            self.file_path_label.setText(f"Selected File: {self.file_path}")
            self.load_gif()

    def load_gif(self):
        gif = self.gif_movie = QMovie(self.file_path)
        self.gif_movie.setScaledSize(QSize(900, 600)) 
        self.image_label.setMovie(self.gif_movie)
        self.gif_movie.start()

    def send_file(self):
        if self.serial_conn and self.serial_conn.is_open:
            # send bluetooth command
            print("Image Sent")
        else:
            print("Serial connection not established.")

def main():
    app = QApplication(sys.argv)
    window = ImageSenderApp()
    window.show()
    sys.exit(app.exec())

if __name__ == "__main__":
    main()