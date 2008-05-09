# Available models:
# eratosthenes leader leader2 loops mobile1 peterson peterson_N pftp snoopy sort

MODELS=eratosthenes.b leader.b leader2.b mobile1.b peterson.b peterson_N.b pftp.b snoopy.b sort.b


%.b : src/%
	./build.sh $^

all: $(MODELS)

clean:
	rm -rf build

distclean:
	for model in $(MODELS); do rm -f $${model}.b; done

.PHONY: all clean distclean
