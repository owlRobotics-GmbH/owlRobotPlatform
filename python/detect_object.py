#!/usr/bin/python3

#
# install with:
#   pip3 install opencv-python
#   pip3 install opencv-contrib-python



import cv2
import time
import numpy as np


IMG_W = 1280  # 320, 640, 1280, 1920, 2560
IMG_H = 720   # 240, 480,  720, 1080,  720

cam = None 
model = None


# Pretrained classes in the model
classNames = {0: 'background',
              1: 'person', 2: 'bicycle', 3: 'car', 4: 'motorcycle', 5: 'airplane', 6: 'bus',
              7: 'train', 8: 'truck', 9: 'boat', 10: 'traffic light', 11: 'fire hydrant',
              13: 'stop sign', 14: 'parking meter', 15: 'bench', 16: 'bird', 17: 'cat',
              18: 'dog', 19: 'horse', 20: 'sheep', 21: 'cow', 22: 'elephant', 23: 'bear',
              24: 'zebra', 25: 'giraffe', 27: 'backpack', 28: 'umbrella', 31: 'handbag',
              32: 'tie', 33: 'suitcase', 34: 'frisbee', 35: 'skis', 36: 'snowboard',
              37: 'sports ball', 38: 'kite', 39: 'baseball bat', 40: 'baseball glove',
              41: 'skateboard', 42: 'surfboard', 43: 'tennis racket', 44: 'bottle',
              46: 'wine glass', 47: 'cup', 48: 'fork', 49: 'knife', 50: 'spoon',
              51: 'bowl', 52: 'banana', 53: 'apple', 54: 'sandwich', 55: 'orange',
              56: 'broccoli', 57: 'carrot', 58: 'hot dog', 59: 'pizza', 60: 'donut',
              61: 'cake', 62: 'chair', 63: 'couch', 64: 'potted plant', 65: 'bed',
              67: 'dining table', 70: 'toilet', 72: 'tv', 73: 'laptop', 74: 'mouse',
              75: 'remote', 76: 'keyboard', 77: 'cell phone', 78: 'microwave', 79: 'oven',
              80: 'toaster', 81: 'sink', 82: 'refrigerator', 84: 'book', 85: 'clock',
              86: 'vase', 87: 'scissors', 88: 'teddy bear', 89: 'hair drier', 90: 'toothbrush'}


def id_class_name(class_id, classes):
    for key, value in classes.items():
        if class_id == key:
            return value




def detectObject(image, filterObjs):
    image_height, image_width, _ = image.shape
    model.setInput(cv2.dnn.blobFromImage(image, size=(300, 300), swapRB=True))
    output = model.forward()
    # print(output[0,0,:,:].shape)
    center_x = 0
    center_y = 0
    top_y = 0

    for detection in output[0, 0, :, :]:
        confidence = detection[2]
        if confidence > .5:
            class_id = detection[1]
            class_name=id_class_name(class_id,classNames)
            if ((filterObjs is not None) and (class_name not in filterObjs)): continue
            center_x = detection[3] + abs(detection[3] - detection[5])/2
            center_y = detection[4] + abs(detection[4] - detection[6])/2
            top_y =  detection[4]

            print(class_name, str(round(confidence,1)), 'center_x', center_x, 'center_y', center_y, 'top_y', top_y)            
            box_x = detection[3] * image_width
            box_y = detection[4] * image_height
            box_width = detection[5] * image_width
            box_height = detection[6] * image_height
            cv2.rectangle(image, (int(box_x), int(box_y)), (int(box_width), int(box_height)), (50, 50, 255), thickness=3)
            cv2.putText(image,class_name + ' ' + str(round(confidence,1)) ,(int(box_x), int(box_y+.1*image_height)),cv2.FONT_HERSHEY_SIMPLEX,(.002*image_width),(0, 0, 255), 10)


    cv2.imshow('objects', image)
    #cv2.waitKey(1)
    return center_x, center_y, top_y
    # cv2.imwrite("image_box_text.jpg",image)		


def captureVideoImage():
    global cam, model
    if cam is None:
        print('opening video device...')
        cam = cv2.VideoCapture(0)
        if cam is None: return None
        print('opened video device')
        cam.set(3, IMG_W)
        cam.set(4, IMG_H)

        time.sleep(0.5)
        print('starting DNN...')
    
        # Loading model
        model = cv2.dnn.readNetFromTensorflow('objdet_models/frozen_inference_graph.pb',
                                      'objdet_models/ssd_mobilenet_v2_coco_2018_03_29.pbtxt')

        model.setPreferableBackend(cv2.dnn.DNN_BACKEND_CUDA)
        model.setPreferableTarget(cv2.dnn.DNN_TARGET_CUDA)
        print('detect_object started')
        time.sleep(0.5)


    ret, img = cam.read()
    if not ret: return None
    return img   
    


if __name__ == '__main__':
    while (cv2.waitKey(1) != 0x1b):
        img = captureVideoImage()
        #img = cv2.imread('test1.jpg')
        detectObject(img, "person")
        time.sleep(0.2) 

