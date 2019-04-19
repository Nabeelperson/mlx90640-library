import httpClient as client 
import sys
import os
import time
import math
import struct
import constants as c 
import supportFunctions as suppfunc
import numpy
import io
import subprocess
import math
sys.path.insert(0, "./build/lib.linux-armv7l-3.5")
import MLX90640 as mlx640

def get640Frame():
    numVals = 768        # number of pixels in a frame (each pixel is 2 bytes)
    frames = list()     # list of c.FRAME_COUNT number of frames that we send in once http req
    oldFrame = True     # used for checking if the current frame is a duplicate
    dt = numpy.dtype('<f4')
    oldBuff = numpy.zeros(numVals, dtype=dt)
    buffRead = numpy.zeros(numVals, dtype=dt)
    
    # get current time with millisecond precision
    ts = time.time()
   
    # if we have not collected a new frame then assign oldBuff just to compare incase we read same frame twice
    if(oldFrame):
        oldBuff = buffRead
        oldFrame = False

    buffRead = mlx640.get_frame()
    hit = suppfunc.hitMarker(buffRead,1)
    if(True):
        try:
            quantizedFrame = suppfunc.mapThermalReadings(buffRead,low_temp,high_temp)
        except ValueError:
            suppfunc.logger.error('NaN values detected')
            NaN_Flag = 1
            for pix in buffRead:
                if math.isnan(pix):
                    pix = 0
                else:
                    pass
        frames.append(quantizedFrame)
        oldFrame = True
    else:
        time.sleep(0.001)

    frame_str=''
    for i in frames:
        frame_str+=i
    return frame_str, ts, hit

def runMLX640(frameRate):
    mlx640.setup(frameRate)
    file_counter=1
    while(True):
        hit = 0
        frames, ts, hit = get640Frame()
        print(ts)
        key = suppfunc.generateKey(thermal_id, frames, ts)
        try:
            if suppfunc.fileChecker():
                suppfunc.logger.info('There are local files to be sent')
                suppfunc.logger.info('Storing locally now')
                suppfunc.storeLocally(frames,ts,hit,file_counter)
                if (p.poll()!=None) or file_counter==1:
                    p = subprocess.Popen(['python3', 'sendLocal.py'], shell=False)
                file_counter+=1
            else:
                pass
                # client.sendFrames(thermal_id, hit, key, host,
                #             port, endpoint, ts, frames)
        except:
            suppfunc.logger.info('Server is down, storing locally')
            suppfunc.storeLocally(frames,ts,hit,file_counter)
            p = subprocess.Popen(['python3', 'sendLocal.py'], shell=False)
            file_counter+=1
 

if __name__ == '__main__':
    # INITIALIZATION
    # Sync NTP
    # ntp = subprocess.Popen(['python3', 'syncNTP.py'], shell=False)
    host = c.BU_HOST
    port = c.BU_PORT
    endpoint  = 'storeframes'
    # client.getConfig()
    import config as conf
    frames = list()
    hit = 0
    low_temp = conf.doorPkg['low_temp']
    high_temp = conf.doorPkg['high_temp']
    thermal_id = conf.doorPkg['tdsId']
    sensors = conf.doorPkg['sensors']
    sensor_counter = 0
    # NaN_Flag = 0
    hit = 0
    if not os.path.isdir("localData"):
        os.mkdir("localData")
    runMLX640(8)
    suppfunc.logger.info("MLX90640 is running")