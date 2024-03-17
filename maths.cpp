#include "maths.hpp"

// nice code got from cpp0x.pl
double round(double fValue) {
    return fValue<0 ? ceil(fValue-0.5) : floor(fValue+0.5);
}

unsigned int calculateCompressionRatio(unsigned int inputSize, unsigned int realSize) {
    return (unsigned int)round(100*((double)inputSize/realSize));
}
