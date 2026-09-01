#if defined(__MINGW64__) || defined(__x86_64__)
    .text
    .globl __chkstk
    .def __chkstk; .scl 2; .type 32; .endef
__chkstk:
    jmp ___chkstk
#endif