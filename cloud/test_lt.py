# chars = [ char for char in 'TutorialsPoint' if char not in 'aeiou']
# print (chars)
'''
lay full data file .txt/ .pkl
'''
import numpy as np
import pickle
# Open your pickle file (replace "your_file.txt" with your actual filename)
with open("/home/trnhong193/Downloads/ntu60_hrnet.pkl", "rb") as f:
    data = pickle.load(f)
# Increase the threshold to display all elements
np.set_printoptions(threshold=np.inf) #important
# Print the keypoint array
print("FRAME")
print(data["annotations"][0]["total_frames"])
print("KEYPOINT")
print(data["annotations"][0]["keypoint"])
print("KEYPOINT_SCORE")
print(data["annotations"][0]["keypoint_score"])

# import json
# import os

# # Load the JSON file
# with open('person_keypoints_default.json') as f:
# data = json.load(f)

# # Define a mapping from keypoints to pairs for skeleton
# skeleton_pairs = data['categories'][0]['skeleton']
# image_info = next((item for item in data['images'] if item['id'] == image_id), None)
# if image_info:
#     # Construct the annotation file path
#     annotation_filename = os.path.splitext(image_info['file_name'])[0] + '.txt'
#     annotation_filepath = os.path.join('/media/khizar_smr/Drive_B/Khizar_data_2/EAD_2/dataset/waqar_working', annotation_filename)
    
#     bbox = annotation['bbox']
#     x_center = (bbox[0] + bbox[2] / 2) / image_info['width']
#     y_center = (bbox[1] + bbox[3] / 2) / image_info['height']
#     width = bbox[2] / image_info['width']
#     height = bbox[3] / image_info['height']
    
#     kp_str = ''
#     for i in range(0, len(keypoints), 3):
#         x = keypoints[i] / image_info['width']
#         y = keypoints[i + 1] / image_info['height']
#         v = keypoints[i + 2]
#         kp_str += f' {x} {y} {v}'

#     yolo_annotation = f'0 {x_center} {y_center} {width} {height}{kp_str}\n'

#     with open(annotation_filepath, 'w') as file:
#         file.write(yolo_annotation)

# # Find the corresponding image info
# image_info = next((item for item in data['images'] if item['id'] == image_id), None)
# if image_info:
#     # Construct the annotation file path
#     annotation_filename = os.path.splitext(image_info['file_name'])[0] + '.txt'
#     annotation_filepath = os.path.join('./labels', annotation_filename)
    
#     # Get the bounding box details
#     bbox = annotation['bbox']
#     x_center = (bbox[0] + bbox[2] / 2) / image_info['width']
#     y_center = (bbox[1] + bbox[3] / 2) / image_info['height']
#     width = bbox[2] / image_info['width']
#     height = bbox[3] / image_info['height']
    
#     # Prepare the keypoints
#     kp_str = ''
#     for i in range(0, len(keypoints), 3):
#         x = keypoints[i] / image_info['width']
#         y = keypoints[i + 1] / image_info['height']
#         v = keypoints[i + 2]
#         kp_str += f' {x} {y} {v}'

#     # Combine class id, bbox and keypoints in YOLO format
#     yolo_annotation = f'0 {x_center} {y_center} {width} {height}{kp_str}\n'

#     # Write to the corresponding annotation file
#     with open(annotation_filepath, 'w') as file:
#         file.write(yolo_annotation)