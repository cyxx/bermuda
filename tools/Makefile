
all: convert_wgp decode_mov decode_ne

convert_wgp: convert_wgp.o
	$(CXX) -o $@ $^ -lz

decode_mov: decode_mov.o
	$(CXX) -o $@ $^

decode_ne: decode_ne.o
	$(CXX) -o $@ $^

clean:
	rm -f *.o
