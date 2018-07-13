all: lib/libilop-hal.so linkkit-example

CC=mipsel-openwrt-linux-gcc

CFLAGS=-Wall -g -O0 -Iinclude -Isrc/hal -D_PLATFORM_IS_LINUX_ -D_GNU_SOURCE
LDFLAGS=-Llib -lilop-tls -lilop-sdk -lilop-hal -lpthread -lrt -ldl

lib/libilop-hal.so: src/hal/base64.c  src/hal/cJSON.c  src/hal/HAL_Crypt_Linux.c  src/hal/HAL_OS_linux.c  src/hal/HAL_TCP_linux.c  src/hal/HAL_TLS_mbedtls.c  src/hal/HAL_UDP_linux.c  src/hal/kv.c
	$(CC) $(CFLAGS) -shared -fPIC -o $@ $^ -Llib -lilop-tls

linkkit-example: src/example/linkkit-example.c src/example/light.c src/hal/cJSON.c
	$(CC) $(CFLAGS) -o $@ $^ -lilop-sdk $(LDFLAGS)

clean:
	rm lib/libilop-hal.so linkkit-example
