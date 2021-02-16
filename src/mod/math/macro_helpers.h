#pragma once

#define STR(x) #x
#define EVAL(a) STR(a)

#define CONCAT2(a,b) a ## _ ##  b
#define EVAL2(a,b) CONCAT2(a,b)
#define CONCAT3(a,b,c) a ## _ ##  b ## _ ## c
#define EVAL3(a,b,c) CONCAT3(a,b,c)
#define CONCAT4(a,b,c,d) a ## _ ##  b ## _ ## c ## _ ## d
#define EVAL4(a,b,c,d) CONCAT4(a,b,c,d)
