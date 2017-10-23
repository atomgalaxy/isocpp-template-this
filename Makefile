all:
	clang++ -std=c++17 ./forward_like.cpp -o forward_like
	! clang++ -std=c++17 \
		-DCOMPILE_FAILING \
		-o forward_like_should_fail \
		./forward_like.cpp
	echo "Success! Those errors are supposed to be there."

clean:
	rm forward_like
