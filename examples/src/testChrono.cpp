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

bool checkDup(float oldFrame[],float newFrame[])
{
    if(oldFrame[0] != newFrame[0]) return true;
    else if (oldFrame[1] != newFrame[128]) return true;
    else return false;
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

    for(int count = 0; count < 300; count++)
    {
        //fb_init();
    auto loop_start = std::chrono::system_clock::now();
    std::cout << loop_start.time_since_epoch().count() << std::endl;
        for(int ii = 0; ii < 16; ii++)
        {
            auto start = std::chrono::system_clock::now();
            
            MLX90640_GetFrameData(MLX_I2C_ADDR, frame);
            
            MLX90640_InterpolateOutliers(frame, eeMLX90640);

            eTa = MLX90640_GetTa(frame, &mlx90640);
            MLX90640_CalculateTo(frame, &mlx90640, emissivity, eTa, mlx90640To);
            
            MLX90640_BadPixelsCorrection((&mlx90640)->brokenPixels, mlx90640To, 1, &mlx90640);
            MLX90640_BadPixelsCorrection((&mlx90640)->outlierPixels, mlx90640To, 1, &mlx90640);
            for(int count = 0; count< 768; count++){
                std::cout << mlx90640To[count] << " ";
            }
            
            auto end = std::chrono::system_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

            // std::cout << elapsed.count() << std::endl;
            // std::cout << "Waiting for: " << frame_time.count() - elapsed.count() << " ms" << std::endl;
            std::this_thread::sleep_for(std::chrono::microseconds(frame_time - elapsed));
        }
    std::cout << std::endl;
    std::cout << 1 << std::endl;
    auto loop_end = std::chrono::system_clock::now();
    // std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(loop_end - loop_start).count() << std::endl;
    }
    
    // outfile.close();
	return 0;
}