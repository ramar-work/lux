/* ------------------------------------------- * 
 * zwalker.c
 * ---------
 * A less error prone way of iterating through
 * strings or unsigned character data.
 *
 * Usage
 * -----
 * ### Building
 *
 *
 * LICENSE
 * -------
 * Copyright 2020 Tubular Modular Inc. dba Collins Design
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * TODO
 * ----
 * 
 * ------------------------------------------- */
#include "zwalker.h"

_Bool memstr (const void * a, const void *b, int size) {
	int32_t ct=0, len = strlen((const char *)b);
	const uint8_t *aa = (uint8_t *)a;
	const uint8_t *bb = (uint8_t *)b;
	_Bool stop=1;
	while (stop) {
		while ((stop = (ct < (size - len))) && memcmp(aa + ct, bb, 1) != 0) { 
			//fprintf(stderr, "%c", aa[ct]);
			ct++; continue; }
		if (memcmp(aa + ct, bb, len) == 0)
			return 1;	
		ct++;
	}
	return 0;	
}

//Return count of occurences of a character in some block.
int32_t memchrocc (const void *a, const char b, int32_t size) {
	_Bool stop=1;
	int32_t ct=0, occ=-1;
	uint8_t *aa = (uint8_t *)a;
	char bb[1] = { b };
	while (stop) {
		occ++;
		while ((stop = (ct < size)) && memcmp(aa + ct, bb, 1) != 0) ct++;
		ct++;
	}
	return occ;
}


//Return count of occurences of a string in some block.
int32_t memstrocc (const void *a, const void *b, int32_t size) {
	_Bool stop=1;
	int32_t ct=0, occ=0;
	uint8_t *aa = (uint8_t *)a;
	uint8_t *bb = (uint8_t *)b;
	int len     = strlen((char *)b);
	while (stop) {
		while ((stop = (ct < (size - len))) && memcmp(aa + ct, bb, 1) != 0) ct++;
		if (memcmp(aa + ct, bb, len) == 0) occ++;
		ct++;
	}
	return occ;
}



//Initialize a block of memory
_Bool memwalk (Mem *mm, uint8_t *data, uint8_t *tokens, int datalen, int toklen) {
#if 0
fprintf(stderr, "Inside memwalk: ");
write(2, data, datalen);
write(2, "\n", 1);
#endif
	int rc    = 0;
	mm->pos   = mm->next;
	mm->size  = memtok(&data[mm->pos], tokens, datalen - (mm->next - 1), toklen);
	if (mm->size == -1) {
	 mm->size = datalen - mm->next;
	}
	mm->next += mm->size + 1;
	//rc      = ((mm->size > -1) && (mm->pos <= datalen));
	rc        = (mm->size > -1);
	mm->chr   = !rc ? 0 : data[mm->next - 1];
	mm->pos  += mm->it;
	mm->size -= mm->it;
#if 0
fprintf(stderr, "rc: %d\n", rc);
fprintf(stderr, "datalen: %d\n", datalen);
fprintf(stderr, "mm->pos: %d\n", mm->pos);
fprintf(stderr, "mm->size: %d\n", mm->size);
#endif
	return rc; 
}


//Where exactly is a substr in memory
int32_t memstrat (const void *a, const void *b, int32_t size)  {
	_Bool stop=1;
	int32_t ct=0;//, occ=0;
	uint8_t *aa = (uint8_t *)a;
	uint8_t *bb = (uint8_t *)b;
	int len     = strlen((char *)b);
	//while (stop = (ct < (size - len)) && memcmp(aa + ct, bb, len) != 0) ct++; 
	while (stop) {
		while ((stop = (ct < (size - len))) && memcmp(aa + ct, bb, 1) != 0) ct++;
		if (memcmp(aa + ct, bb, len) == 0)
			return ct; 
		ct++;
	}
	return -1;
}


//Where exactly is a substr in memory
int32_t memchrat (const void *a, const char b, int32_t size) {
	_Bool stop=1;
	int32_t ct=0;// occ=0;
	uint8_t *aa = (uint8_t *)a;
	//uint8_t *bb = (uint8_t *)b;
	char bb[1] = { b };
	//while (stop = (ct < (size - len)) && memcmp(aa + ct, bb, len) != 0) ct++; 
	while ((stop = (ct < size)) && memcmp(aa + ct, bb, 1) != 0) ct++;
	return (ct == size) ? -1 : ct;
}


//Finds the 1st occurence of one char, Keep running until no tokens are found in range...
int32_t memtok (const void *a, const uint8_t *tokens, int32_t sz, int32_t tsz) 
{
	int32_t p=-1,n;
	
	for (int i=0; i<tsz; i++)
	#if 1
		p = ((p > (n = memchrat(a, tokens[i], sz)) && n > -1) || p == -1) ? n : p;
	#else
	{
		p = ((p > (n = memchrat(a, tokens[i], sz)) && n > -1) || p == -1) ? n : p;
		fprintf(stderr, "found char %d at %d\n", tokens[i], memchrat(a, tokens[i], sz));
		nmprintf("p is", p);
	}
	#endif
	
	return p;
}


//Finds the first occurrence of a complete token (usually a string). 
//keep running until no more tokens are found.
int32_t memmatch (const void *a, const char *tokens, int32_t sz, char delim) {
	int32_t p=-1, n, occ = -1;

	/*Check that the user has supplied a delimiter. (or fail in the future)*/
	if (!(occ = memchrocc(tokens, delim, strlen(tokens))))
		return -1 /*I found nothing, sorry*/;

	/*Initialize a temporary buffer for each copy*/
	int t = 0; 
	char buf[strlen(tokens) - occ];
	memset(&buf, 0, strlen(tokens) - occ);

	/*Loop through each string in the token list*/
	while (t < strlen(tokens) && (n = memtok(&tokens[t], (uint8_t *)"|\0", sz, 2)) > -1) {
		/*Copy to an empty buffer*/
		memcpy(buf, &tokens[t], n);
		buf[n] = '\0';
		t += n + 1;

		/*This should find the FIRST occurrence of what we're looking for within block*/
		p = ((p > (n = memstrat(a, buf, sz)) && n > -1) || p == -1) ? n : p;
		/*fprintf(stderr, "found str %s at %d\n", buf, memstrat(a, buf, sz)); nmprintf("p is", p);*/
		memset(&buf, 0, strlen(tokens) - occ);
	}
	return p;
}


/*Copy strings*/
char *memstrcpy (char *dest, const uint8_t *src, int32_t len) {
	memcpy(dest, src, len);
	dest[len]='\0';
	return dest;
}
