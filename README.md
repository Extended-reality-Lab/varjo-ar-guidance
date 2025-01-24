Overview

In this readme we will go over how to send frames using the varjoSDK

Getting the frames

In order to obtain the frames from the varjo headset, we must first intialize a DataStreamer object. This is done using:
    std::unique_ptr<VarjoExamples::DataStreamer> m_streamer;

Later in the code you will need to start your DataStreamer so that it can begin collecting frames using:
m_streamer = std::make_unique<DataStreamer>(m_session, std::bind(&TestClient::onFrameReceived, this, std::placeholders::_1));
m_streamer->startDataStream(varjo_StreamType_DistortedColor, varjo_TextureFormat_NV12, varjo_ChannelFlag_Left);

The &TestClient::onFrameReceived is the function that will occur everytime a new frame is collected

Later in the code you will need to create a TestClient object with your onFrameReceived function which should look something like:
void onFrameReceived(const DataStreamer::Frame& frame)
    {

    }

Now within the function, frame represents the current frame and you can use it as needed.

About the Frame

The frame matrix is stored in frame.data.data()

You can also access metadata about the frame using frame.metadata.

Frame data is transposed so,
Width =  frame.metadata.bufferMetadata.height
Height = frame.metadata.bufferMetadata.rowStride

The frame data is also stored as NV12 color data so, if using opencv, you will need to convert from NV12 to BGR using:
 bgr_image = cv2.cvtColor(yuv_image, cv2.COLOR_YUV2BGR_NV12)