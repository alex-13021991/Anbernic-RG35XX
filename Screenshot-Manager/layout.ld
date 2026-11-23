/* Reproduce the read-only data layout of the manager shipped on the RG35XX. */
SECTIONS {
    .rodata : {
        *(.rodata.str1.1)
        *(SORT_BY_NAME(.rodata.sm.*))
        *(.rodata)
    }
}
INSERT BEFORE .eh_frame;
