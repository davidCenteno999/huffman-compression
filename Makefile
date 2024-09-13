CC = gcc
CFLAGS = -Wall -g
COMPRESSOR = compression
DECOMPRESSOR = decompression

all: compress decompress

compress: $(COMPRESSOR).c
	$(CC) $(COMPRESSOR).c -o $(COMPRESSOR) 
	./$(COMPRESSOR)

decompress: $(DECOMPRESSOR).c
	$(CC) $(DECOMPRESSOR).c -o $(DECOMPRESSOR) 
	./$(DECOMPRESSOR)


clean:
	rm -f $(COMPRESSOR) $(DECOMPRESSOR)
