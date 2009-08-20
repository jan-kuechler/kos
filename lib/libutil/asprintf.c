/*
 * Copyright (c) 2006-2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Burkhard Weseloh.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "stdlib.h"
#include "string.h"
#include "types.h"

#include "jprintf.h"

#define ASPRINTF_INITIAL_BUFFER_SIZE 64

struct asprintf_args
{
    char * buffer;
    size_t buflen;
    size_t bytes_written;
};

int asprintf_putc(struct asprintf_args * arg, char c)
{
    if(arg->bytes_written == arg->buflen - 1)
    {
        arg->buflen = arg->buflen * 2;
        arg->buffer = (char*)realloc(arg->buffer, arg->buflen * 2);
        if(arg->buffer == NULL)
        {
            return -1;
        }
    }

    arg->buffer[arg->bytes_written++] = c;

    return 1;
}

int vasprintf(char ** buffer, const char * format, va_list ap)
{
    struct asprintf_args args;
    struct jprintf_args asnprintf_handler = { (pfn_putc)&asprintf_putc, 0, (void*)&args };
    int retval;
    
    if(buffer == NULL)
    {
        return -1;
    }

    args.buffer = (char*)malloc(ASPRINTF_INITIAL_BUFFER_SIZE);
    if(args.buffer == NULL)
    {
        return -1;
    }
    
    args.buflen = ASPRINTF_INITIAL_BUFFER_SIZE;
    args.bytes_written = 0;

	retval = jvprintf(&asnprintf_handler, format, ap);

    // string Null-terminieren
    if(asprintf_putc(&args, 0) == -1)
    {
        return -1;
    }

    if(args.bytes_written < args.buflen)
    {
        // Puffer nicht größer als nötig.
        args.buffer = (char*)realloc(args.buffer, args.bytes_written);
    }

    *buffer = args.buffer;

    return retval;
}

int asprintf(char ** buffer, const char * format, ...)
{
	va_list ap;
	int retval;

	va_start(ap, format);
	retval = vasprintf(buffer, format, ap);
	va_end(ap);

	return retval;
}
