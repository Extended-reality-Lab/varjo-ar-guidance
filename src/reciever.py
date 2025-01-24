import zmq
import cv2
import numpy as np
import time

import frame_pb2


def receive_message():
    context = zmq.Context()
    subscriber = context.socket(zmq.SUB)
    
    subscriber.connect("tcp://localhost:5555")
    
    subscriber.setsockopt_string(zmq.SUBSCRIBE, "")
    
    print("Receiver is listening for messages...")


    width = 640
    stride = 896  

    frame_count = 0
    start_time = time.time()  

    try:
        while True:
            message = subscriber.recv()
            
            
            frame = frame_pb2.Frame()
            frame.ParseFromString(message)

            if len(frame.intrinsics.distortionCoefficients) == 0:
                print("it's empty")


            img_data = np.frombuffer(frame.pixels, dtype=np.uint8)
            
      
            height = width + width // 2

            
            yuv_image = img_data.reshape((height, stride))
            
           
            expected_size = stride * width * 3 // 2
            if len(img_data) != expected_size:
                print(f"Received incorrect image size: {len(img_data)} bytes, expected {expected_size} bytes")
                continue 

            try:
                
                bgr_image = cv2.cvtColor(yuv_image, cv2.COLOR_YUV2BGR_NV12)
                
                if bgr_image is not None:
                    
                    bgr_image = bgr_image[:, :-100]
                    
                    cv2.imshow("Received Image", bgr_image)
                    
                    # Press 'q' to quit the program
                    key = cv2.waitKey(1) & 0xFF
                    if key == ord('q'): 
                        break
                else:
                    print("Failed to decode image.")
            except cv2.error as e:
                print(f"OpenCV error: {e}")

            
            frame_count += 1

          
            elapsed_time = time.time() - start_time
            if elapsed_time >= 1.0:  
                fps = frame_count / elapsed_time
                print(f"FPS: {fps:.2f}")

                
                frame_count = 0
                start_time = time.time()

    except KeyboardInterrupt:
        print("Receiver interrupted, shutting down.")
    finally:
        subscriber.close()
        context.term()
        cv2.destroyAllWindows()


if __name__ == "__main__":
    receive_message()
