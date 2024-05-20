import sys
from PySide6.QtWidgets import QApplication, QMainWindow, QPushButton, QFileDialog, QVBoxLayout, QWidget, QLabel, QComboBox, QMessageBox, QLineEdit
from PySide6.QtCore import Slot, QTimer
import cv2
import numpy as np
import paho.mqtt.client as mqtt
from PIL import Image, ImageDraw, ImageFont

<<<<<<< HEAD
=======

>>>>>>> 428f9d541bb6aa0066c741d445aee994b6e4c1b2
# client address
# client = "localhost" 
# client = "172.20.10.14"
# client = "192.168.137.1"
client = "192.168.0.100"
<<<<<<< HEAD
=======

>>>>>>> 428f9d541bb6aa0066c741d445aee994b6e4c1b2

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("GUI")
        self.resize(400, 360)
<<<<<<< HEAD
=======

>>>>>>> 428f9d541bb6aa0066c741d445aee994b6e4c1b2

        self.choose_button = QPushButton("Choose Image", self)
        self.choose_button.clicked.connect(self.choose_image)
        self.choose_button.setGeometry(50, 30, 150, 50)
<<<<<<< HEAD
=======

>>>>>>> 428f9d541bb6aa0066c741d445aee994b6e4c1b2

        self.send_button = QPushButton("Send Image", self)
        self.send_button.clicked.connect(self.send_image)
        self.send_button.setGeometry(220, 30, 150, 50)
<<<<<<< HEAD
=======

>>>>>>> 428f9d541bb6aa0066c741d445aee994b6e4c1b2

        self.image_label = QLabel("", self)
        self.image_label.setGeometry(50, 80, 320, 30)

<<<<<<< HEAD
=======

>>>>>>> 428f9d541bb6aa0066c741d445aee994b6e4c1b2
        self.choose_button = QPushButton("Choose Animation", self)
        self.choose_button.clicked.connect(self.choose_animation)
        self.choose_button.setGeometry(50, 130, 150, 50)

<<<<<<< HEAD
=======

>>>>>>> 428f9d541bb6aa0066c741d445aee994b6e4c1b2
        self.send_button = QPushButton("Send Animation", self)
        self.send_button.clicked.connect(self.send_animation)
        self.send_button.setGeometry(220, 130, 150, 50)

<<<<<<< HEAD
=======

>>>>>>> 428f9d541bb6aa0066c741d445aee994b6e4c1b2
        self.num_frames_label = QLabel("Number of Frames:", self)
        self.num_frames_label.setGeometry(50, 190, 150, 30)
        self.num_frames_input = QLineEdit(self)
        self.num_frames_input.setGeometry(220, 190, 100, 30)

<<<<<<< HEAD
        self.animation_label = QLabel("", self)
        self.animation_label.setGeometry(50, 220, 320, 30)

=======

        self.animation_label = QLabel("", self)
        self.animation_label.setGeometry(50, 220, 320, 30)


>>>>>>> 428f9d541bb6aa0066c741d445aee994b6e4c1b2
        self.mode_combo = QComboBox(self)
        self.mode_combo.addItems(["0 display off", "1 display image", "2 display animation", "3 display distance"])
        self.mode_combo.setGeometry(50, 270, 150, 50)

<<<<<<< HEAD
        self.send_mode_button = QPushButton("Send Mode", self)
        self.send_mode_button.clicked.connect(self.send_mode)
        self.send_mode_button.setGeometry(220, 270, 150, 50)
=======

        self.send_mode_button = QPushButton("Send Mode", self)
        self.send_mode_button.clicked.connect(self.send_mode)
        self.send_mode_button.setGeometry(220, 270, 150, 50)

>>>>>>> 428f9d541bb6aa0066c741d445aee994b6e4c1b2

        self.file_path = ""
        self.mqttc = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        self.mqttc.on_connect = self.on_connect
        self.mqttc.on_message = self.on_message
        self.mqttc.connect(client, port=1883)
        self.mqttc.loop_start()
        
        self.counter = 0
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.publish_counter)
        self.timer.start(1000)
<<<<<<< HEAD
=======


    # def on_connect(self, client, userdata, flags, reason_code, properties):
    #     if reason_code.is_failure:
    #         print(f"Failed to connect: {reason_code}. loop_forever() will retry connection")
    #     else:
    #         print("Connected to MQTT broker")

>>>>>>> 428f9d541bb6aa0066c741d445aee994b6e4c1b2

    def on_connect(self, client, userdata, flags, reason_code, properties):
        if reason_code.is_failure:
            print(f"Failed to connect: {reason_code}. loop_forever() will retry connection")
        else:
            client.subscribe("distance")
            print("Connected to MQTT broker")

<<<<<<< HEAD
=======

>>>>>>> 428f9d541bb6aa0066c741d445aee994b6e4c1b2
    def on_message(self, client, userdata, message):
        print(f"Received message with topic '{message.topic}'")
        return
        text = message.payload.decode()

<<<<<<< HEAD
=======

>>>>>>> 428f9d541bb6aa0066c741d445aee994b6e4c1b2
        with Image.open("black.png") as im:
            draw = ImageDraw.Draw(im)
            font = ImageFont.truetype("Roboto-Regular.ttf", 16)
            draw.text((450, 320), text, fill =(255, 255, 255), font=font)
            
            image = np.array(im.convert('RGB'))
<<<<<<< HEAD
=======
            # resize to correct ratio of display
            # image = cv2.resize(image, (960, 720))
            # iamge = np.array(image)
            # draw = ImageDraw.Draw(image)
            # draw.text((450, 320), text, fill =(255, 255, 255))
            # split image into 9 equal parts
>>>>>>> 428f9d541bb6aa0066c741d445aee994b6e4c1b2
            h, w, channels = image.shape
            part_h = h // 3
            part_w = w // 3
            for row in range(3):
                for col in range(3):
                    part = image[row * part_h: (row + 1) * part_h, col * part_w: (col + 1) * part_w]
                    topic = f"img{row * 3 + col + 1}"
                    self.mqttc.publish(topic, part.tobytes(), qos=1)
        print("Published distance")

<<<<<<< HEAD
=======

>>>>>>> 428f9d541bb6aa0066c741d445aee994b6e4c1b2
    @Slot()
    def choose_image(self):
        file_dialog = QFileDialog(self)
        file_dialog.setNameFilter("Files (*.png *.jpg *.jpeg)")
        if file_dialog.exec():
            self.file_path = file_dialog.selectedFiles()[0]
            self.image_label.setText(f"Selected Image: {self.file_path}")

<<<<<<< HEAD
=======

>>>>>>> 428f9d541bb6aa0066c741d445aee994b6e4c1b2
    def send_image(self):
        with Image.open(self.file_path) as im:
            image = np.array(im.convert('RGB'))
            # resize to correct ratio of display
            image = cv2.resize(image, (960, 720))
            # split image into 9 equal parts
            h, w, channels = image.shape
            part_h = h // 3
            part_w = w // 3
            for row in range(3):
                for col in range(3):
                    part = image[row * part_h: (row + 1) * part_h, col * part_w: (col + 1) * part_w]
                    topic = f"img{row * 3 + col + 1}"
                    self.mqttc.publish(topic, part.tobytes(), qos=1)
            print(f"Image of {len(image.tobytes())} bytes sent.")

<<<<<<< HEAD
=======

>>>>>>> 428f9d541bb6aa0066c741d445aee994b6e4c1b2
    @Slot()
    def choose_animation(self):
        file_dialog = QFileDialog(self)
        file_dialog.setNameFilter("Files (*.gif *.mp4)")
        if file_dialog.exec():
            self.file_path = file_dialog.selectedFiles()[0]
            self.animation_label.setText(f"Selected Image: {self.file_path}")
            print(f"Selected File: {self.file_path}")

<<<<<<< HEAD
=======

>>>>>>> 428f9d541bb6aa0066c741d445aee994b6e4c1b2
    def send_animation(self):
        # num_key_frames = 12
        num_key_frames = int(self.num_frames_input.text()) if self.num_frames_input.text() else 6
        with Image.open(self.file_path) as im:
            for i in range(num_key_frames):
                im.seek(im.n_frames // num_key_frames * i)
                frame = np.array(im.convert('RGB'))
                # resize to correct ratio of display
                frame = cv2.resize(frame, (960, 720))
                # split image into 9 equal parts
                h, w, channels = frame.shape
                part_h = h // 3
                part_w = w // 3
                for row in range(3):
                    for col in range(3):
                        part = frame[row * part_h: (row + 1) * part_h, col * part_w: (col + 1) * part_w]
                        topic = f"anim{row * 3 + col + 1}"
                        self.mqttc.publish(topic, bytes([i]) + part.tobytes(), qos=1)
                print(f"Frame {i+1} parts sent successfully.")
            print(f"Animation with {num_key_frames} frames sent.")
            
    def send_mode(self):
        selected_mode = self.mode_combo.currentText()
        self.mqttc.publish("mode", selected_mode[:1], qos=1)
        print(f"Mode {selected_mode[:1]} sent.")

<<<<<<< HEAD
=======

>>>>>>> 428f9d541bb6aa0066c741d445aee994b6e4c1b2
    @Slot()
    def publish_counter(self):
        self.counter += 1
        self.mqttc.publish("heartbeat", str(self.counter), qos=1)
<<<<<<< HEAD
=======

>>>>>>> 428f9d541bb6aa0066c741d445aee994b6e4c1b2

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec())