#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <math.h>
#include "imu.h"

#define CONN_PORT        5000
#define CONN_MAX_QUEUE   10

#define ACCEL_SCALE 2.0/32767.0
#define GYRO_SCALE  250.0/32767.0


#define GYRO_X_OFFSET -116.549
#define GYRO_Y_OFFSET 56.346
#define GYRO_Z_OFFSET -12.395

/*  
#define GYRO_X_OFFSET 61.98
#define GYRO_Y_OFFSET 27.80
#define GYRO_Z_OFFSET 54.97
 */

#define STR_MAX_LEN 2048

int main(int argc, char** argv){
    imu_t imu;
    int listenfd = 0;
    int connfd = 0;
    struct sockaddr_in serv_addr;
    int err = -1;
    int gyro[3] = {0, 0, 0};
    int accel[3] = {0, 0, 0};
    float eulers[3] = {0.0,0.0,0.0};
    float quat[4] = {0.0,0.0,0.0,0.0};
    char* welcomeStr = "Imu conv";
    char recvStr[STR_MAX_LEN] = "";
    char sendStr[STR_MAX_LEN] = "";
    char* token;
    char inputValues[6][1024] = {"","","","","",""};
    int i = 0;
    int connected = 0;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(CONN_PORT);

    err = bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if(err < 0){
            fprintf(stderr,"\tERR: Error in bind function: [%s]\n", strerror(err));
            return -1;
    }

    err = listen(listenfd, CONN_MAX_QUEUE);
    if(err < 0){
        fprintf(stderr,"\tERR: Error in listen function: [%s]\n", strerror(err));
        return -1;
    }

    imu_set_state(&imu, IMU_STATE_READY);

    while(1){
        if(connected == 0){
            imu = imu_init();

            imu_set_calibration_mode(&imu, IMU_CALIBMODE_NEVER);
            imu_set_gyro_scale_factor(&imu, GYRO_SCALE);
            imu_set_accelerometer_scale_factor(&imu, ACCEL_SCALE);
            imu.gyro_offset.x = GYRO_X_OFFSET;
            imu.gyro_offset.y = GYRO_Y_OFFSET;
            imu.gyro_offset.z = GYRO_Z_OFFSET;

            connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
            if(connfd < 0){
                fprintf(stderr,"\tERR: Error in accept: [%s]\n", strerror(connfd));
                return -1;
            }

            connected = 1;

            write(connfd, welcomeStr, strlen(welcomeStr));
        }

        err = read(connfd, recvStr, STR_MAX_LEN);
        if(err < 0){
            fprintf(stderr,"\tERR: Error in reading from socket: [%s]\n", strerror(err));
            return -1;
        }else if(err == 0){
            printf("\tClient disconnected! Waiting for connection...\n");
            connected = 0;
            continue;
        }

        i = 0;
        token = strtok(recvStr, ",");
        while(token != NULL){
            if(i < 6){
                strcpy(inputValues[i],token);

                token = strtok(NULL, ",");

                i++;
            }
        }

        for(i = 0; i < 3; i++){
            gyro[i] = atoi(inputValues[i]);
            accel[i] = atoi(inputValues[i+3]);
        }

        printf("accel = (%d,%d,%d)\n"
               "gyro = (%d,%d,%d)\n",
               accel[0], accel[1], accel[2],
               gyro[0], gyro[1], gyro[2]);

        imu_set_accelerometer_raw(&imu, accel[0], accel[1], accel[2]);
        imu_set_gyro_raw(&imu, gyro[0], gyro[1], gyro[2]);
        imu_main_loop(&imu);

        quat[0]   = imu.orientation_quat.w;
        quat[1]   = imu.orientation_quat.x;
        quat[2]   = imu.orientation_quat.y;
        quat[3]   = imu.orientation_quat.z;
        eulers[0] = imu.orientation.roll*180*M_1_PI;
        eulers[1] = imu.orientation.pitch*180*M_1_PI;
        eulers[2] = imu.orientation.yaw*180*M_1_PI;

        snprintf(sendStr, STR_MAX_LEN, 
                 "Q%f,%f,%f,%fE%f,%f,%f\n"
                 quat[0],quat[1],quat[2],quat[3],
                 eulers[0],eulers[1],eulers[2]);

        write(connfd, sendStr, strlen(sendStr));

        printf("\n\tquaternions = %.3f,%.3f,%.3f,%.3f\n"
               "\n\teulers = %.3f,%.3f,%.3f\n",
               quat[0],quat[1],quat[2],quat[3],
               eulers[0],eulers[1],eulers[2]);
    }
 
    close(connfd);

    return 0;
}
