#include <string.h>

#define WASM_EXPORT(f) __attribute__((export_name(#f))) f

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif

#include "wasm_json_ui.hpp"

// The UI class will serve as a Meta as well. We're not using subclasses/virtuals,
// so typedef instead of subclass.
using Meta = JSONUI;
using UI = JSONUI;
class dsp {};

// we'll require macro declarations for FAUST_INPUTS, FAUST_OUTPUTS, etc.
#define FAUST_UIMACROS

// We won't need these though
#define FAUST_ADDBUTTON(l,f)
#define FAUST_ADDCHECKBOX(l,f)
#define FAUST_ADDVERTICALSLIDER(l,f,i,a,b,s)
#define FAUST_ADDHORIZONTALSLIDER(l,f,i,a,b,s)
#define FAUST_ADDNUMENTRY(l,f,i,a,b,s)
#define FAUST_ADDVERTICALBARGRAPH(l,f,a,b)
#define FAUST_ADDHORIZONTALBARGRAPH(l,f,a,b)

#define SAMPLE_COUNT 128

//**************************************************************
// Intrinsic
//**************************************************************

<<includeIntrinsic>>


// We skip using virtual methods because we don't need them and the binary will be
// smaller without them. We hackishly #define virtual to be empty. We make sure
// to include the following headers first, though, because the generated mydsp class
// will include them as well and we need virtual to be defined before that.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <math.h>

#define virtual

<<includeclass>>

#undef virtual

//**************************************************************
// Interface
//**************************************************************

mydsp _instance;

// Enough space for the input and output buffers, plus the arrays of pointers to them
unsigned char _iobuffers[
      (FAUST_INPUTS + FAUST_OUTPUTS) * sizeof(FAUSTFLOAT*)
    + (FAUST_INPUTS + FAUST_OUTPUTS) * SAMPLE_COUNT * sizeof(FAUSTFLOAT)
];

extern "C" {

// Avoid pulling in all sorts of IO (printf, etc) from the default error handling for calling pure virtual functions.
// Usually unnecessary vtables will get stripped out anyway, but... just in case.
void __cxa_pure_virtual(void) {
    abort();
}

// This is for zeroing out the stack during wasm-ctor-eval.
extern unsigned char __global_base;

void WASM_EXPORT(_init_json_ui)() {
    JSONUI jui{};
    _instance.buildUserInterface(&jui);
    _instance.metadata(&jui);

    // Allocate a preliminary buffer on the stack to write the JSON to. npf_snprintf needs a non-volatile buffer,
    // and in order to later write to address 0 we'll need that to be volatile. It's okay to use a lot of memory here
    // because we only call this during compile time (i.e. during wasm-ctor-eval) and then we strip this function out.
    constexpr size_t BUF_SIZE = 4 * 4096;
    char _buf[BUF_SIZE];
    jui.JSON(_buf, FAUST_CLASS_NAME, FAUST_FILE_NAME, FAUST_INPUTS, FAUST_OUTPUTS, _iobuffers);

    // Write _buf to address 0. This will be in the unused portion of the stack so it won't overwrite anything important
    volatile unsigned char* _zero = 0;
    size_t i = 0;
    for (; i < BUF_SIZE; ++i) {
        _zero[i] = _buf[i];
    }

    // Zero out the rest of the stack. Again, this function is getting called by wasm-ctor-eval, and we do this
    // so that the dirty stack contents don't get saved into the resulting binary.
    for (; _zero + i < &__global_base; ++i) {
        _zero[i] = 0;
    }
}

void WASM_EXPORT(init)(void*, int sample_rate) {
    _instance.init(sample_rate);
}

void WASM_EXPORT(compute)(void*, int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) {
    _instance.compute(count, inputs, outputs);
}

}
