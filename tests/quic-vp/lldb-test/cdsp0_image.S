        .text
        .global main
main:
        r0 = #0x11111111
break1:
        r0 = #0x22222222
break2:
        r0 = #0x33333333
break3:
        r2 = #0
        stop(r1)
