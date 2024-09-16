CC = gcc
CFLAGS = -Wall -g
COMPRESSOR = compression
COMPRESSOR1 = compression_pthread
COMPRESSOR2 = compression_pipe
DECOMPRESSOR = decompression
DECOMPRESSOR2 = decompression_pthread
DECOMPRESSOR3 = decompression_pipe

all: compress1 compress2 compress3 decompress1 decompress2 decompress3

compress1: $(COMPRESSOR).c
	$(CC) $(COMPRESSOR).c -o $(COMPRESSOR)
	./$(COMPRESSOR)

compress2: $(COMPRESSOR1).c
	$(CC) $(COMPRESSOR1).c -o $(COMPRESSOR1)
	./$(COMPRESSOR1)

compress3: $(COMPRESSOR2).c
	$(CC) $(COMPRESSOR2).c -o $(COMPRESSOR2)
	./$(COMPRESSOR2)

decompress1: $(DECOMPRESSOR).c
	$(CC) $(DECOMPRESSOR).c -o $(DECOMPRESSOR)
	./$(DECOMPRESSOR)

decompress2: $(DECOMPRESSOR1).c
	$(CC) $(DECOMPRESSOR1).c -o $(DECOMPRESSOR1)
	./$(DECOMPRESSOR1)

decompress3: $(DECOMPRESSOR2).c
	$(CC) $(DECOMPRESSOR2).c -o $(DECOMPRESSOR2)
	./$(DECOMPRESSOR2)


clean:
	rm -f $(COMPRESSOR) $(COMPRESSOR1) $(COMPRESSOR2) $(DECOMPRESSOR) $(DECOMPRESSOR1) $(DECOMPRESSOR2)
