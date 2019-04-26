// duration_cast
#include <iostream>     // std::cout
#include <chrono>       // std::chrono::seconds, std::chrono::milliseconds
                        // std::chrono::duration_cast
#include <thread>
#include "headers/MLX90640_API.h"
#include <fstream>
//#include "lib/fb.h"


#define MLX_I2C_ADDR 0x33
#define FPS 16
#define FRAME_TIME_MICROS (1000000/FPS)
#define OFFSET_MICROS 850
#define LOWTEMP 15.0
#define HIGHTEMP 40.0


// Maps thermal values to a single byte, consistant with what is in 
// the python code
int quantize(float f)
{
    if(f < LOWTEMP) return 0;
    else if (f > HIGHTEMP) return 255;
    else return (f - LOWTEMP)/(HIGHTEMP - LOWTEMP) * 255;
}

int hit(float *temp)
{
    for(int ii = 0; ii < 764; ii+=4)
    {
        if(*(temp + ii) > 27.0) return 1;
    }
    return 0;
}

int main ()
{
    // std::ofstream outfile;
    // outfile.open("data.txt");
	static uint16_t eeMLX90640[832];
    float emissivity = 1;
    uint16_t frame[834];
    static float image[768];
    static float mlx90640To[768];
    static float oldFrame[2];
    float eTa;
    static uint16_t data[768*sizeof(float)];
    int frame_counter = 0;
    int hitMarker = 0;

    auto frame_time = std::chrono::microseconds(FRAME_TIME_MICROS + OFFSET_MICROS);
    // std::cout << frame_time.count() << std::endl;

	MLX90640_SetDeviceMode(MLX_I2C_ADDR, 0);
    MLX90640_SetSubPageRepeat(MLX_I2C_ADDR, 0);

	// ONLY for 16 FPS
	MLX90640_SetRefreshRate(MLX_I2C_ADDR, 0b101);
	MLX90640_SetChessMode(MLX_I2C_ADDR);

	paramsMLX90640 mlx90640;
    MLX90640_DumpEE(MLX_I2C_ADDR, eeMLX90640);
    MLX90640_ExtractParameters(eeMLX90640, &mlx90640);

    while(true)
    {

    auto loop_start = std::chrono::system_clock::now();
        for(int ii = 0; ii < 16; ii++)
        {
            auto start = std::chrono::system_clock::now();
            
            MLX90640_GetFrameData(MLX_I2C_ADDR, frame);
            
            MLX90640_InterpolateOutliers(frame, eeMLX90640);

            eTa = MLX90640_GetTa(frame, &mlx90640);
            MLX90640_CalculateTo(frame, &mlx90640, emissivity, eTa, mlx90640To);
            
            MLX90640_BadPixelsCorrection((&mlx90640)->brokenPixels, mlx90640To, 1, &mlx90640);
            MLX90640_BadPixelsCorrection((&mlx90640)->outlierPixels, mlx90640To, 1, &mlx90640);

            // Check for the hitmarker
            if(!hitMarker) hitMarker = hit(mlx90640To);

            // Quantize the temperatures and write the hex of the byte to the stdout
            for(int count = 0; count< 768; count++){
                char buffer[2];
                sprintf(buffer, "%.2x", quantize(mlx90640To[count]));
                std::cout << buffer;
            }
            
            // Figure out how long to wait
            auto end = std::chrono::system_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            std::this_thread::sleep_for(std::chrono::microseconds(frame_time - elapsed));
        }

        // Send the frames, the hit marker and timestamp to the python code
        std::cout<< ',' 
            << hitMarker << ',' 
            << std::chrono::duration_cast<std::chrono::milliseconds>(loop_start.time_since_epoch()).count()
            << std::endl;
    }
	return 0;
}