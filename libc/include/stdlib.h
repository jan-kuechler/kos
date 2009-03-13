/*
 * Copyright (c) 2006 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Antoine Kaufmann.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed by the tyndur Project
 *     and its contributors.
 * 4. Neither the name of the tyndur Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _STDLIB_H_
#define _STDLIB_H_

#include <stddef.h>
#include "string.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

/*
void exit(int result);
void abort(void);
int atexit(void (*function)(void));
*/

void* malloc(size_t size);
void free(void* ptr);
void* realloc(void* ptr, size_t size);
void* calloc(size_t size, size_t count);

/** !!!!!!!! **/
#if 0

long strtol(const char* str, char** endptr, int base);
unsigned long strtoul(const char* str, char** endptr, int base);
char* getenv(const char* name);
int setenv(const char* name, const char* value, int overwrite);

/**
 * Umgebungsvariable setzen
 *
 * @param str String in der Form variable=wert
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int putenv(const char* str);

void unsetenv(const char* name);

void qsort(void *base, size_t num, size_t size, int (*comparator)(const void *, const void *));

char* mktemp(char* template);

#ifndef CONFIG_LIBC_NO_STUBS
double atof(const char* str);
#endif
int abs(int x);

int system(const char* command);

/**
 * Anzahl der Bytes die das erste Zeichen belegt
 *
 * @param s     Pointer auf den Anfang den Anfang des Zeichens
 * @param slen  Maximale Laenge die das Zeichen haben kann (Stringlaenge)
 *
 * @return Laenge des Zeichens oder -1 wenn ein Fehler auftritt (z.B.
 *         ungueltiges Zeichen)
 */
int mblen(const char* s, size_t slen);

/**
 * Erstes Zeichen im String in einen wchar umwandeln. Wird NULL als wc
 * uebergeben, gibt die Funktion lediglich die Laenge des Zeichens zurueck
 * (mblen). Ist s NULL gibt die Funktion 0 zurueck.
 *
 * @param wc    Pointer auf den wchar in dem das Ergebnis abgelegt werden soll
 * @param s     Pointer auf den Anfang des Zeichens
 * @param len   Maximale Laenge die das Zeichen haben kann (Stringlaenge)
 *
 * @return Bei Erfolg wird die Anzahl der benutzten Bytes aus s zurueckgegeben,
 *         im Fehlerfall -1
 */
int mbtowc(wchar_t* wc, const char* s, size_t len);


/**
 * Seed fuer Zufallszahlgenerator setzen
 *
 * @param seed Seed
 */
void srand(unsigned int seed);

/**
 * Zufallszahl generieren
 *
 * @return Zufallszahl
 */
int rand(void);

/**
 * Zufallszahl generieren
 *
 * @return Zufallszahl
 */
long int random(void);

/**
 * Seed fuer Zufallszahlgenerator setzen
 *
 * @param seed Seed
 */
void srandom(unsigned int seed);

#endif /* 0 */

#endif
