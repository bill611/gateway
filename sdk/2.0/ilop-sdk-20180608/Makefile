all: lib/libilop-hal.so linkkit-example

CC=arm-linux-gcc

CFLAGS=-Wall -g -O0 -Iinclude -Llib  -Isrc/hal
LDFLAGS=-lilop-tls -lilop-sdk -lilop-hal -lpthread -lrt

lib/libilop-hal.so: src/hal/base64.c  src/hal/cJSON.c  src/hal/HAL_Crypt_Linux.c  src/hal/HAL_OS_linux.c  src/hal/HAL_TCP_linux.c  src/hal/HAL_TLS_mbedtls.c  src/hal/HAL_UDP_linux.c  src/hal/kv.c
	$(CC) $(CFLAGS) -shared -fPIC -o $@ $^ -lilop-tls

linkkit-example: src/example/linkkit-example.c src/example/light.c src/hal/cJSON.c
	$(CC) -muclibc $(CFLAGS) -o $@ $^ -lilop-sdk $(LDFLAGS)

clean:
	rm lib/libilop-hal.so linkkit-example
