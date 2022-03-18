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


// We'll put the following union at memory 0, so that the JSON description will end
// up there where the js module expects it. The section(".rodata") is a hack to make
// sure that this memory gets placed before all other global data -- wasm-ld
// puts .rodata before everything else.
union JsonAndDsp {
    char _json_data[4 * 4096];
    struct {
        mydsp _dsp_instance;
        // Enough space for the input and output buffers, plus the arrays of
        // pointers to them
        unsigned char _iobuffers[
              (FAUST_INPUTS + FAUST_OUTPUTS) * sizeof(FAUSTFLOAT*)
            + (FAUST_INPUTS + FAUST_OUTPUTS) * SAMPLE_COUNT * sizeof(FAUSTFLOAT)
        ];
    };
};
JsonAndDsp __attribute__((section(".rodata"))) _instance;


extern "C" {

// Avoid pulling in all sorts of IO (printf, etc) from the default error handling for calling pure virtual functions.
// Usually unnecessary vtables will get stripped out anyway, but... just in case.
void __cxa_pure_virtual(void) {
    abort();
}

// This is for zeroing out the stack during wasm-ctor-eval.
extern unsigned char __data_end;
extern unsigned char __heap_base;

void WASM_EXPORT(_init_json_ui)() {
    JSONUI jui{};
    _instance._dsp_instance.buildUserInterface(&jui);
    _instance._dsp_instance.metadata(&jui);

    jui.JSON(
        _instance._json_data,
        FAUST_CLASS_NAME,
        FAUST_FILE_NAME,
        FAUST_INPUTS,
        FAUST_OUTPUTS,
        /* dspSize */ (uintptr_t)_instance._iobuffers - (uintptr_t)&_instance._dsp_instance
    );

    // Zero out the rest of the stack. Again, this function is getting called by wasm-ctor-eval, and we do this
    // so that the dirty stack contents don't get saved into the resulting binary.
    volatile unsigned char* _start = (volatile unsigned char*)&__data_end;
    for (size_t i = 0; _start + i < &__heap_base; ++i) {
        _start[i] = 0;
    }
}

void WASM_EXPORT(init)(void*, int sample_rate) {
    _instance._dsp_instance.init(sample_rate);
}

void WASM_EXPORT(compute)(void*, int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) {
    _instance._dsp_instance.compute(count, inputs, outputs);
}

}
