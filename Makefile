CC = gcc
DEPS = imu.h imu_algebra.h imu_constants.h imu_math.h imu_types.h imu_utils.h
OBJ = imu.o imu_algebra.o imu_math.o imu_utils.o
LIBS = -lm
DBG = 0

%.o: %.c $(DEPS)
ifeq ($(DBG),1)
	$(CC) -O0 -ggdb -c -o $@ $<
else
	$(CC) -c -o $@ $<
endif

imuConv: $(OBJ)
ifeq ($(DBG),1)
	$(CC) -O0 -ggdb -o $@ $^ $(LIBS)
else
	$(CC) -o $@ $^ $(LIBS)
endif

clean:
	rm ./*.o
