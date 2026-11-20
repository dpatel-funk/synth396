# make test           run every stage
# make stage N=3      run only stage 3 (see README for the stage list)
test: run_tests
	./run_tests
stage: run_tests
	./run_tests $(N)
run_tests: run_tests.cpp ../DSP_funcs/src/dsp/*.h ../DSP_funcs/src/engine/*.h
	g++ -std=c++17 -O2 -Wall -Wextra -o run_tests run_tests.cpp -lm
clean:
	rm -f run_tests
