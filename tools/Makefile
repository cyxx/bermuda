
all: convert_wgp decode

convert_wgp: convert_wgp.o
	$(CXX) -o $@ $^

decode: decode.o
	$(CXX) -o $@ $^

clean:
	rm -f *.o
