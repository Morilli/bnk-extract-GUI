#ifndef GNU_MINMAX_H
#define GNU_MINMAX_H

#ifndef __cplusplus
#ifdef max
#undef max
#endif
#define max(a, b) __extension__ ({ \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a > _b ? _a : _b; \
})

#ifdef min
#undef min
#endif
#define min(a, b) __extension__ ({ \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a < _b ? _a : _b; \
})
#endif

#endif
