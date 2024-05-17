import sys
from PySide6.QtWidgets import QApplication, QMainWindow, QPushButton, QFileDialog, QVBoxLayout, QWidget, QLabel
from PySide6.QtCore import Slot
import cv2
import numpy as np
import paho.mqtt.client as mqtt
from PIL import Image

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Image Sender")
        self.resize(400, 250)

        self.choose_button = QPushButton("Choose Image", self)
        self.choose_button.clicked.connect(self.choose_image)
        self.choose_button.setGeometry(50, 50, 150, 50)

        self.send_button = QPushButton("Send Image", self)
        self.send_button.clicked.connect(self.send_image)
        self.send_button.setGeometry(220, 50, 150, 50)

        self.image_label = QLabel("", self)
        self.image_label.setGeometry(50, 120, 320, 30)

        self.file_path = ""
        self.mqttc = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        self.mqttc.on_connect = self.on_connect
        # self.mqttc.connect("localhost", port=1883)
        self.mqttc.connect("172.20.10.14", port=1883)
        self.mqttc.loop_start()

    def on_connect(self, client, userdata, flags, reason_code, properties):
        if reason_code.is_failure:
            print(f"Failed to connect: {reason_code}. loop_forever() will retry connection")
        else:
            print("Connected to MQTT broker")

    @Slot()
    def choose_image(self):
        file_dialog = QFileDialog(self)
        file_dialog.setNameFilter("Files (*.png *.jpg *.jpeg *.gif *.mp4)")
        if file_dialog.exec():
            self.file_path = file_dialog.selectedFiles()[0]
            self.image_label.setText(f"Selected Image: {self.file_path}")
            print(f"Selected File: {self.file_path}")

            # num_key_frames = 12
            # with Image.open(self.file_path) as im:
            #     for i in range(num_key_frames):
            #         im.seek(im.n_frames // num_key_frames * i)
            #         im.save('{}.jpg'.format(i))
            # imageObject = Image.open(self.file_path)
            # print(imageObject.n_frames)


    # below working
    # @Slot()
    # def send_image(self):
    #     if self.file_path:
    #         with open(self.file_path, 'rb') as file:
    #             filecontent = file.read()
    #             byteArr = bytearray(filecontent)
    #             img = cv2.imread(self.file_path, cv2.COLOR_BGR2RGB)
    #             h, w, channels = img.shape
    #             part_h = h // 3
    #             part_w = w // 3
                
    #             for i in range(3):
    #                 for j in range(3):
    #                     part = img[i * part_h: (i + 1) * part_h, j * part_w: (j + 1) * part_w]
    #                     # _, buffer = cv2.imencode('.jpg', part)
    #                     # byte_array = buffer.tobytes()
    #                     topic = f"img{i * 3 + j + 1}"  # Topics are img1 to img9
    #                     # self.mqttc.publish(topic, bytes([0]) + byte_array, qos=1)

    #                     self.mqttc.publish(topic, part.tobytes(), qos=1)
    #                     # print(len(byte_array))
    #             print("Image parts sent successfully.")
    #     else:
    #         print("Please choose an image first.")

    # @Slot()
    # def send_image(self):
    #     num_key_frames = 12
    #     with Image.open(self.file_path) as im:
    #         for i in range(num_key_frames):
    #             im.seek(im.n_frames // num_key_frames * i)
    #             frame = np.array(im.convert('RGB'))
    #             h, w, channels = frame.shape
    #             part_h = h // 3
    #             part_w = w // 3

    #             for row in range(3):
    #                 for col in range(3):
    #                     part = frame[row * part_h: (row + 1) * part_h, col * part_w: (col + 1) * part_w]
    #                     topic = f"img{row * 3 + col + 1}"
    #                     self.mqttc.publish(topic, part.tobytes(), qos=1)
    #                     print(len(part.tobytes()))

    #             print(f"Frame {i} parts sent successfully.")

    #         print("All frames processed.")
    def send_image(self):

        num_key_frames = 12
        with Image.open(self.file_path) as im:
            for i in range(num_key_frames):
                im.seek(im.n_frames // num_key_frames * i)
                frame = np.array(im.convert('RGB'))
                frame = cv2.resize(frame, (960, 720))
                h, w, channels = frame.shape
                part_h = h // 3
                part_w = w // 3

                for row in range(3):
                    for col in range(3):
                        part = frame[row * part_h: (row + 1) * part_h, col * part_w: (col + 1) * part_w]
                        topic = f"img{row * 3 + col + 1}"
                        self.mqttc.publish(topic, part.tobytes(), qos=1)
                        print(len(part.tobytes()))
                print(f"Frame {i} parts sent successfully.")

            print("All frames processed.")


    # @Slot()
    # def send_image(self):
    #     if self.file_path:
    #         with open(self.file_path, 'rb') as file:
    #             filecontent = file.read()
    #             topic = "img9"  # Topic for sending GIF
    #             self.mqttc.publish(topic, filecontent, qos=1)
    #             print("GIF sent successfully.")
    #     else:
    #         print("Please choose a GIF file first.")

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec())
