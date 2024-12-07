import zmq
import cv2
import numpy as np


def receive_message():
    context = zmq.Context()
    subscriber = context.socket(zmq.SUB)
    
    subscriber.connect("tcp://localhost:5555")
    
    # Subscribe to all messages
    subscriber.setsockopt_string(zmq.SUBSCRIBE, "")
    
    print("Receiver is listening for messages...")

    # Set image resolution (adjust as necessary)
    width = 608
    stride = 896  

    try:
        while True:
            message = subscriber.recv()
            
            img_data = np.frombuffer(message, dtype=np.uint8)
            
            yuv_image = img_data.reshape((width + width // 2, stride))
            
            # Expected size of the image based on resolution and NV12 format
            expected_size = stride * width * 3 // 2
            if len(img_data) != expected_size:
                print(f"Received incorrect image size: {len(img_data)} bytes, expected {expected_size} bytes")
                continue  

            try:
                # Convert NV12 to BGR using OpenCV (YUV 4:2:0 semi-planar to BGR)
                bgr_image = cv2.cvtColor(yuv_image, cv2.COLOR_YUV2BGR_NV12)
                
                if bgr_image is not None:
                    # Slice the image to remove the last 50 pixels horizontally
                    bgr_image = bgr_image[:, :-100]
                    
                    cv2.imshow("Received Image", bgr_image)
                    
                    key = cv2.waitKey(1) & 0xFF
                    if key == ord('q'): 
                        break
                else:
                    print("Failed to decode image.")
            except cv2.error as e:
                print(f"OpenCV error: {e}")
            
    except KeyboardInterrupt:
        print("Receiver interrupted, shutting down.")
    finally:
        subscriber.close()
        context.term()
        cv2.destroyAllWindows()

if __name__ == "__main__":
    receive_message()
