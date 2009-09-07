.global _start

.extern __init /* in libkos */

_start: call __init
.wait:  jmp  .wait
