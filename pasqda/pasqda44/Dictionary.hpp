// Dictionary.hpp - part of Word Replacing Transformation by P.Skibinski

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HASH_TABLE_SIZE		(1<<20)
#define HASH_DOUBLE_MULT	37
#define HASH_MULT			23
int word_hash[HASH_TABLE_SIZE];

int sizeDict=0;
unsigned char** dict=NULL;
unsigned char* dictlen=NULL;
unsigned char* dictmem;

int ngram_hash[256][256];




int reservedSet[256]; 
int addSymbols[256]; 
int sym2codeword[256]; 
int codeword2sym[256]; 

int dictionary,dict1size,dict2size,dict3size,dict4size,dict1plus2plus3,dict1plus2;
int bound4,bound3,dict123size,dict12size;



// convert upper string to lower
inline void toLower(unsigned char* s,int s_size)
{
	for (int i=0; i<s_size; i++)
		s[i]=TOLOWER(s[i]);
}


// convert lower string to upper
inline void toUpper(unsigned char* s,int s_size)
{
	for (int i=0; i<s_size; i++)
		s[i]=TOUPPER(s[i]); 
}


// make hash from string
inline unsigned int stringHash(const unsigned char *ptr, int len)
{
	unsigned int hash;
	for (hash = 0; len>0; len--, ptr++)
		hash = HASH_MULT * hash + *ptr;
 
	return hash&(HASH_TABLE_SIZE-1);
}


// check if word "s" does exist in the dictionary using hash "h" 
inline int checkHashExactly(const unsigned char* s,int s_size,int h)
{
	int i;

	i=word_hash[h];
	if (i>0)
	{
		if (dictlen[i]!=s_size || memcmp(dict[i],s,s_size)!=0)
		{
			i=word_hash[(h+s_size*HASH_DOUBLE_MULT)&(HASH_TABLE_SIZE-1)];
			if (i>0)
			{
				if (dictlen[i]!=s_size || memcmp(dict[i],s,s_size)!=0)
				{
					i=word_hash[(h+s_size*HASH_DOUBLE_MULT*HASH_DOUBLE_MULT)&(HASH_TABLE_SIZE-1)];
					if (i>0)
					{
						if (dictlen[i]!=s_size || memcmp(dict[i],s,s_size)!=0)
							i=-1;
					}
					else
						i=-1;
				}
			}
			else
				i=-1;
		}
	}
	else
		i=-1;
	
	if (i>=dictionary)
		i=-1;

	return i;
}

// check if word "s" (prefix of original word) does exist in the dictionary using hash "h" 
inline int checkHash(const unsigned char* s,int s_size,int h)
{
	int i;


	i=word_hash[h];
	if (i>0)
	{
		if (dictlen[i]>s_size || memcmp(dict[i],s,s_size)!=0)
		{
			i=word_hash[(h+s_size*HASH_DOUBLE_MULT)&(HASH_TABLE_SIZE-1)];
			if (i>0)
			{
				if (dictlen[i]>s_size || memcmp(dict[i],s,s_size)!=0)
				{
					i=word_hash[(h+s_size*HASH_DOUBLE_MULT*HASH_DOUBLE_MULT)&(HASH_TABLE_SIZE-1)];
					if (i>0)
					{
						if (dictlen[i]>s_size || memcmp(dict[i],s,s_size)!=0)
							i=-1;
					}
					else
						i=-1;
				}
			}
			else
				i=-1;
		}
	}
	else
		i=-1;

	if (i>=dictionary)
		i=-1;
	
	return i;
}


// check if word "s" or prefix of word "s" does exist in the dictionary using hash "h" 
inline int findShorterWord(const unsigned char* s,int s_size)
{
	int ret;
	int i;
	int best;
	unsigned int hash;

	hash = 0;
	for (i=0; i<WORD_MIN_SIZE+tryShorterBound; i++)
		hash = HASH_MULT * hash + s[i];
 
	best=-1;
	for (i=WORD_MIN_SIZE+tryShorterBound; i<s_size; i++)
	{
		ret=checkHash(s,i,hash&(HASH_TABLE_SIZE-1));	
		if (ret>=0)
			best=ret;
		hash = HASH_MULT*hash + s[i];
	}

	return best;
}

inline int findShorterWordRev(const unsigned char* s,int s_size)
{
	int ret;
	int i;

	for (i=s_size-1; i>=WORD_MIN_SIZE+tryShorterBound; i--)
	{
		ret=checkHash(s+s_size-i,i,stringHash(s+s_size-i,i));	
		if (ret>=0)
			return ret;
	}

	return -1;
}

inline void encodeCodeWord(int i,FILE* fileout);

// encode word (should be lower case) using n-gram array (when word doesn't exist in the dictionary)
inline void encodeAsText(unsigned char* s,int s_size,FILE* fileout)
{
	int i,ngram;

	{
		ngram=0;
		for (i=0; i<s_size; )
		{
			if (IF_OPTION(OPTION_USE_NGRAMS) && IF_OPTION(OPTION_CAPTIAL_CONVERSION))
				ngram=ngram_hash[s[i]][s[i+1]];

			if (ngram>0 && ngram<dict1size)
			{
				encodeCodeWord(ngram,fileout);
				i+=2;
			}
			else
			{
				ENCODE_PUTC(s[i],fileout);
				i++;
			}
		}
	}
}




inline void encodeCodeWord(int i,FILE* fileout)
{
	int first,second,third,fourth;
	int no=i;

	first=i-1;



	if (first>=bound4)
	{
		first-=bound4;

		fourth=first/dict123size;
		first=first%dict123size;
		third=first/dict12size;		
		first=first%dict12size;
		second=first/dict1size;		
		first=first%dict1size;

		ENCODE_PUTC(sym2codeword[dict1plus2plus3+fourth],fileout);
		PRINT_CODEWORDS(("1st=%d ",sym2codeword[dict1plus2plus3+fourth]));

		ENCODE_PUTC(sym2codeword[dict1plus2+third],fileout);
		PRINT_CODEWORDS(("2nd=%d ",sym2codeword[dict1plus2+third]));

		ENCODE_PUTC(sym2codeword[dict1size+second],fileout);
		PRINT_CODEWORDS(("3rd=%d ",sym2codeword[dict1size+second]));
 
		ENCODE_PUTC(sym2codeword[first],fileout);
		PRINT_CODEWORDS(("4th=%d ",sym2codeword[first]));
	}
	else
	if (first>=bound3)
	{
		first-=bound3;

		third=first/dict12size;		
		first=first%dict12size;
		second=first/dict1size;		
		first=first%dict1size;

		ENCODE_PUTC(sym2codeword[dict1plus2+third],fileout);
		PRINT_CODEWORDS(("1st=%d ",sym2codeword[dict1plus2+third]));

		ENCODE_PUTC(sym2codeword[dict1size+second],fileout);
		PRINT_CODEWORDS(("2nd=%d ",sym2codeword[dict1size+second]));

		ENCODE_PUTC(sym2codeword[first],fileout);
		PRINT_CODEWORDS(("3rd=%d ",sym2codeword[first]));
	}
	else
		if (first>=dict1size)
		{
			first-=dict1size;

			second=first/dict1size;		
			first=first%dict1size;

			ENCODE_PUTC(sym2codeword[dict1size+second],fileout);
			PRINT_CODEWORDS(("1st=%d ",sym2codeword[dict1size+second]));

			ENCODE_PUTC(sym2codeword[first],fileout);
			PRINT_CODEWORDS(("2nd=%d ",sym2codeword[first]));
		}
		else
		{
			ENCODE_PUTC(sym2codeword[first],fileout);
			PRINT_CODEWORDS(("1st=%d ",sym2codeword[first]));
		}

		PRINT_CODEWORDS((" no=%d\n", no-1,dict[no]));
}





// encode word "s" using dictionary
inline void encodeWord(FILE* fileout,unsigned char* s,int s_size,EWordType wordType)
{
	int i;
	int size=0;
	int flagToEncode=-1;


	if (s_size<1)
	{
		return;
	}

	s[s_size]=0;

	if (wordType!=LOWERWORD)
	{
		if (IF_OPTION(OPTION_CAPTIAL_CONVERSION))
		{
		if (wordType==FIRSTUPPER)
		{
			flagToEncode=CHAR_FIRSTUPPER;
			s[0]=TOLOWER(s[0]);
		}
		else // wordType==UPPERWORD
		{
			flagToEncode=CHAR_UPPERWORD;
			toLower(s,s_size);
		}
		}
		else
			wordType=LOWERWORD;
	}
	

	if (IF_OPTION(OPTION_USE_DICTIONARY))
	{
		i=checkHashExactly(s,s_size,stringHash(s,s_size));

		if (i<0 && IF_OPTION(OPTION_TRY_SHORTER_WORD))
		{
			// try to find shorter version of word in dictionary
			i=findShorterWord(s,s_size);

			if (i>=0)
				size=dictlen[i];
			else
				{
					i=findShorterWordRev(s,s_size);

					if (i>=0)
					{
						if (wordType!=LOWERWORD)
						{
							ENCODE_PUTC(flagToEncode,fileout);
							if (IF_OPTION(OPTION_SPACE_AFTER_CC_FLAG))
								ENCODE_PUTC(' ',fileout);
							wordType=LOWERWORD;
						}
						
						s[s_size-dictlen[i]]=0;
						encodeAsText(s,s_size-dictlen[i],fileout);
						s+=dictlen[i];
						s_size-=dictlen[i];
					}
				}
		}
	}
	else
		i=-1;




	if (i>=0)
	{
		if (wordType!=LOWERWORD)
		{
			ENCODE_PUTC(flagToEncode,fileout);
			if (IF_OPTION(OPTION_SPACE_AFTER_CC_FLAG))
				ENCODE_PUTC(' ',fileout);
		}

		encodeCodeWord(i,fileout);

		if (size>0)
			encodeAsText(s+size,s_size-size,fileout);
	}
	else
	{
		if (wordType!=LOWERWORD)
		{
			ENCODE_PUTC(flagToEncode,fileout);
			if (IF_OPTION(OPTION_SPACE_AFTER_CC_FLAG))
				ENCODE_PUTC(' ',fileout);
		}
 
		encodeAsText(s,s_size,fileout);
	}
}


// decode word using dictionary
#define DECODE_WORD(dictNo,i)\
{\
		switch (dictNo)\
		{\
			case 4:\
				i+=bound4;\
				break;\
			case 3:\
				i+=bound3;\
				break;\
			case 2:\
				i+=dict1size;\
				break;\
		}\
\
\
		if (i>=0 && i<sizeDict)\
		{\
			i++;\
			s_size=dictlen[i];\
			memcpy(s,dict[i],s_size+1);\
		}\
		else\
		{\
			printf("File is corrupted!\n");\
			exit(2);\
		}\
}



inline int decodeCodeWord(FILE* file, unsigned char* s,int& c)
{
	int i,dictNo,s_size;



	if (codeword2sym[c]<dict1size)
	{
		i=codeword2sym[c];
		dictNo=1;
		DECODE_WORD(dictNo, i);
		return s_size;
	}
	else
	if (codeword2sym[c]<dict1plus2)
		i=dict1size*(codeword2sym[c]-dict1size);
	else
	if (codeword2sym[c]<dict1plus2plus3)
		i=dict12size*(codeword2sym[c]-dict1plus2);
	else
		i=dict123size*(codeword2sym[c]-dict1plus2plus3);



	DECODE_GETC(c,file);



	if (codeword2sym[c]<dict1size)
	{
		i+=codeword2sym[c];
		dictNo=2;
		DECODE_WORD(dictNo, i);
		return s_size;
	}
	else
	if (codeword2sym[c]<dict1plus2)
		i+=dict1size*(codeword2sym[c]-dict1size);
	else
		i+=dict12size*(codeword2sym[c]-dict1plus2);

	DECODE_GETC(c,file);



	if (codeword2sym[c]<dict1size)
	{
		i+=codeword2sym[c];
		dictNo=3;
		DECODE_WORD(dictNo, i);
		return s_size;
	}
	else
	if (codeword2sym[c]<dict1plus2)
		i+=dict1size*(codeword2sym[c]-dict1size);


	DECODE_GETC(c,file);


	if (codeword2sym[c]<dict1size)
		i+=codeword2sym[c];
	else 
		printf("error\n");




	dictNo=4;
	DECODE_WORD(dictNo, i);

	return s_size;
}

unsigned char* loadDictionary(FILE* file,unsigned char* mem,int word_count)
{
	unsigned char* word;
	int i,j,c,collision,bound;

	collision=0;
	bound=sizeDict+word_count;


	while (!feof(file))
	{
		word=mem;
		do
		{
			c=getc(file);

			word[0]=c;
			word++;
		}
		while (c>32);

		if (c==EOF)
			break;
		if (c=='\r') 
			c=getc(file);
		
		word[-1]=0;
		i=word-mem-1;
		
		dictlen[sizeDict]=i;
		dict[sizeDict]=mem;



		j=stringHash(mem,i);
		mem+=(i/4+1)*4;
		

		if (word_hash[j]!=0)
		{
			c=(j+i*HASH_DOUBLE_MULT)&(HASH_TABLE_SIZE-1);
			if (word_hash[c]!=0)
			{
				c=(j+i*HASH_DOUBLE_MULT*HASH_DOUBLE_MULT)&(HASH_TABLE_SIZE-1);
				if (word_hash[c]!=0)
				{
					collision++;
//					printf("collision=%s\n",dict[sizeDict]);
				}
				else
				{
					if (dictlen[sizeDict]==2)
						ngram_hash[dict[sizeDict][0]][dict[sizeDict][1]]=sizeDict;
					word_hash[c]=sizeDict++;
				}
			}
			else
			{
				if (dictlen[sizeDict]==2)
					ngram_hash[dict[sizeDict][0]][dict[sizeDict][1]]=sizeDict;
				word_hash[c]=sizeDict++;
			}
		}
		else
		{
			if (dictlen[sizeDict]==2)
				ngram_hash[dict[sizeDict][0]][dict[sizeDict][1]]=sizeDict;
			word_hash[j]=sizeDict++;
		}



		
		if (sizeDict>dictionary || sizeDict>=bound)
		{
			sizeDict--;
			break;
		}
		
	}

	if (collision>0 && collision!=1)
		printf("warning: hash collisions=%d\n",collision);

	return mem;
}



void WRT_initializeCodeWords()
{
	int c,charsUsed;

	for (c=0; c<256; c++)
	{
		addSymbols[c]=0;
		codeword2sym[c]=0;
		sym2codeword[c]=0;
		reservedSet[c]=0;
	}

	for (c=BINARY_FIRST; c<=BINARY_LAST; c++)
		addSymbols[c]=1;


	for (c=0; c<256; c++)
	{
		if ((IF_OPTION(OPTION_USE_DICTIONARY) && addSymbols[c])
			|| c==CHAR_ESCAPE || c==CHAR_LOWERWORD || c==CHAR_FIRSTUPPER || c==CHAR_UPPERWORD || c==CHAR_CR_LF
			|| (IF_OPTION(OPTION_WORD_SURROROUNDING_MODELING) && c==CHAR_PUNCTUATION))
			reservedSet[c]=1;
		else
			reservedSet[c]=0;
	}

	if (IF_OPTION(OPTION_USE_DICTIONARY))
	{
		charsUsed=0;
		for (c=0; c<256; c++)
		{
			if (addSymbols[c])
			{
				codeword2sym[c]=charsUsed;
				sym2codeword[charsUsed]=c;
				charsUsed++;
			}
		}


		dict2size=32;
		dict3size=16;
		dict4size=16;
		dict1size=charsUsed-dict2size-dict3size-dict4size;

		dictionary=(dict1size*dict2size*dict3size*dict4size+dict1size*dict2size*dict3size+dict1size*dict2size+dict1size);
		bound4=dict1size*dict2size*dict3size+dict1size*dict2size+dict1size;
		bound3=dict1size*dict2size+dict1size;
		dict123size=dict1size*dict2size*dict3size;
		dict12size=dict1size*dict2size;
		dict1plus2=dict1size+dict2size;
		dict1plus2plus3=dict1size+dict2size+dict3size;


		if (dictlen==NULL && dict==NULL)
		{
			dict=(unsigned char**)calloc(sizeof(unsigned char*)*(dictionary+1),1);
			
			dictlen=(unsigned char*)calloc(sizeof(unsigned char)*(dictionary+1),1);
		}

		PRINT_DICT(("%d %d %d %d charsUsed=%d sizeDict=%d\n",dict1size,dict2size,dict3size,dict4size,charsUsed,sizeDict));
	}
}

void WRT_deinitialize();

// read dictionary from files to arrays
bool initialize(unsigned char* dictName,unsigned char* shortDictName,bool encoding)
{
	int fileLen;
	FILE* file;
	unsigned char* mem;

	WRT_deinitialize();
	dict=NULL;
	dictlen=NULL;
	dictmem=NULL;

	memset(&word_hash[0],0,HASH_TABLE_SIZE*sizeof(word_hash[0]));
	memset(ngram_hash,0,sizeof(ngram_hash));

	if (dictName==NULL && shortDictName==NULL)
	{
		WRT_initializeCodeWords();
		sizeDict=0;
		return true;
	}

	if (dictName==NULL && shortDictName!=NULL)
	{
		dictName=shortDictName;
		shortDictName=NULL;
	}



	if (IF_OPTION(OPTION_USE_DICTIONARY))
	{
		printf("- loading dictionary %s\n",dictName); 

		file=fopen((const char*)dictName,"rb");
		if (file==NULL)
		{
			printf("Can't open dictionary %s\n",dictName);
			return false;
		}

		fileLen=flen(file);

	
		dictmem=(unsigned char*)calloc(fileLen*2,1);
		if (dictmem==NULL)
		{
			printf("Not enough memory!\n");
			exit(0);
		}

		mem=dictmem;
		sizeDict=1;

		WRT_initializeCodeWords();

		if (dict==NULL || dictlen==NULL)
			return false;

		mem=loadDictionary(file,mem,dictionary);



		fclose(file);

		PRINT_DICT(("1st word=%s 2=%s 3=%s\n",dict[1],dict[2],dict[3]));

		printf("- loaded dictionary %d/%d words\n",sizeDict,dictionary);
	}
	else
	{
		WRT_initializeCodeWords();
		sizeDict=0;
	}


	return true;
}


void WRT_deinitialize()
{
/*	FILE* fw=fopen("pasqda2.dic","wb");

	if (fw)
	for (int i=0; i<sizeDict; i++)
		fprintf(fw,"%s\n",dict[i]);
*/
	if (dict)
		free(dict);
	if (dictlen)
		free(dictlen);
	if (dictmem)
		free(dictmem);
}
