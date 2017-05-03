# SDN SVC streaming framework.

This SDN SVC streaming framework is based on Dr. Chih-Heng Ke's myEvalSVC-Mininet. [[1]]  
I modified the streamer and receiver to let SVC streaming send packets on 3 UDP ports (total 3 layers of SVC stream). By doing so, SDN controller can setup flow tables for each layer of SVC stream (based on different ports) and separate their routing path for better transmission quality.

The basic work flow of this SDN SVC streaming testing framework is to encoded the video into a 3-layer h264-svc video. Then, stream this SVC via 3 UDP ports to client in mininet environment. After the streaming finished, decode the h264-svc back to yuv and compare with source yuv video by PSNR value.

## Requirement
1. JSVM installed.
2. SVEF-Mod installed.  based on Scalable Video-streaming Evaluation Framework
3. Download myfixyuv.c for fixing the distorted video.
4. Download the testing raw YUV video, i.e. foreman_cif.yuv. 


  [1]: http://csie.nqu.edu.tw/smallko/sdn/myEvalSVC-Mininet.htm

## Using this framework
1. Prepare the network topology with 1 host as a streamer and another host as the client, with mininet connect with both to simulate the network topology.
2. Prepare the temporal encoding/decoding configuration file.

##### temproal_main.cfg
	OutputFile       temporal.264
	FrameRate      30.0
	FramesToBeEncoded 300
	GOPSize           4
	BaseLayerMode      2
	IntraPeriod      4
	 
	SearchMode    4
	SeachRange    32
	NumLayers      1
	LayerCfg  temporal_layer0.cfg

##### temporal_layer0.cfg
	InputFile  foreman_cif.yuv
	SourceWidth   352
	SourceHeight  288
	FrameRateIn   30
	FrameRateOut        30

### 3. Encoding

```
./H264AVCEncoderLibTestStatic -pf temproal_main.cfg > temporal_encoding.txt
```
![Alt text](/docs/images/image002.jpg)


### 4. Decode the temporal.264 record the decoding process into temporal_originaldecoderoutput.txt. We need some information from it.
```
./H264AVCDecoderLibTestStatic temporal.264 temporal.yuv > temporal_originaldecoderoutput.txt
```
![Alt text](/docs/images/image003.jpg)

### 5. Use the JSVM BitStreamExtractor to generate the original NALU trace file (temporal_originaltrace.txt)
```
./BitStreamExtractorStatic -pt temporal_originaltrace.txt temporal.264
```
![Alt text](/docs/images/image006.jpg)

### 6.   Use f-nstamp to add the frame-number in temporal_originaltrace.txt. (It will generate temporal_originaltrace-frameno.txt)
```
mytools/svef-1.5/f-nstamp temporal_originaldecoderoutput.txt temporal_originaltrace.txt > temporal_originaltrace-frameno.txt
```
![Alt text](/docs/images/image007.jpg)

### 7.  Setup the network topology using Mininet and create h1 as the server and h2 as the client.

### 8. On the client termianl (h2)
```
sudo ./receiver 4455 myout.264 15000 > myreceivedtrace.txt
```


### 9. On the server terminal. (h1)
```
sudo ./streamer temporal_originaltrace-frameno.txt 30 10.0.0.2 4455 temporal.264 1 > mysent.txt
```
