
all: convert_wgp decode_mov

convert_wgp: convert_wgp.o
	$(CXX) -o $@ $^ -lz

decode_mov: decode_mov.o
	$(CXX) -o $@ $^

clean:
	rm -f *.o
