// Detect.hpp - part of Word Replacing Transformation by P.Skibinski 

int langSum;

inline void checkWord(unsigned char* s,int s_size)
{
	if (s_size<=WORD_MIN_SIZE+1)
		return;

	s[s_size]=0;

	int i=checkHashExactly(s,s_size,stringHash(s,s_size));

	if (i>=0)
	{
		langSum++;
	}

	return;
}

void WRT_reset_options();

int WRT_detectFileType(char* filename,FILE* file, int part_length, int parts)
{
	unsigned char s[1024];
	int i,last_c,c,flen,length;
	int s_size,binCount,EOLCount,EOLCountBad,punctCount,punctCountBad,spaceAfterLF,fftell;

	s_size=0;
	binCount=0;
	EOLCount=0;
	EOLCountBad=0;
	punctCount=0;
	punctCountBad=0;
	spaceAfterLF=0;
	last_c=0;
	c=32;
	langSum=0;

	WRT_reset_options();

	fseek(file, 0, SEEK_END );
	flen=ftell(file)-part_length;

	if (part_length*parts>flen)
	{
		parts=1;
		part_length+=flen;
		flen=0;
	}

	for (i=1; i<=parts; i++)
	{
		fseek(file, (flen/parts)*i, SEEK_SET );
		fftell=(flen/parts)*i;
		length=part_length;

		while (length>0 && c>=0)
		{
			length--;
			 

			if ((c<32 || (c>=BINARY_FIRST)) && c!=9 && c!=10 && c!=13)
				binCount++;

			
			if ((c>='a' && c<='z') || (c<='Z' && c>='A'))
			{

				c=TOLOWER(c);
				
				s[s_size++]=c;
				if (s_size>=sizeof(s)-1)
				{
					checkWord(s,s_size);
					s_size=0;
				}
				last_c=c;
				c=getc(file);
				fftell++;
				continue;
			}
			
			
			checkWord(s,s_size);
			s_size=0;
			
			if (c=='\r')
			{
				c=fgetc(file);
				fftell++;
			}

			if (c=='\n')
			{	
				c=fgetc(file);
				fftell++;

				if (c==' ')
				{
					c=fgetc(file);
					fftell++;

					if (c != ' ')
						spaceAfterLF++;
				}

				c=TOLOWER(c);
				last_c=TOLOWER(last_c);

				if (last_c>='a' && last_c<='z' && c>='a' && c<='z')
					EOLCount++;
				else
					EOLCountBad++;

				last_c='\n';
			}
			else
			{
				if (c>' ') 
				{
					if ((last_c>='a' && last_c<='z') || (last_c>='A' && last_c<='Z'))
						punctCount++;
					else
						punctCountBad++;
				}
				last_c=c;
			}

			c=fgetc(file);
			fftell++;
		}
	}

	fseek(file, 0, SEEK_END );
	flen=ftell(file);

	fseek(file, 0, SEEK_SET );


	if (parts==1)
		part_length*=2;

#ifdef CALGARY_CORPUS_OPTIM
	if (langSum<part_length*parts/120)
#else
	if (langSum<part_length*parts/150 || binCount>part_length*parts/150) // 1024/3
#endif
		TURN_OFF(OPTION_USE_DICTIONARY);


	if (!IF_OPTION(OPTION_USE_DICTIONARY) && binCount>part_length*parts/15) // 3333
	{
		if (!forceWordSurroroundModeling)
			TURN_OFF(OPTION_WORD_SURROROUNDING_MODELING);
		if (!forceNormalTextFilter)
			TURN_OFF(OPTION_NORMAL_TEXT_FILTER);
	}



	if (punctCount*2<punctCountBad*3)
		if (!forceWordSurroroundModeling)
			TURN_OFF(OPTION_WORD_SURROROUNDING_MODELING);


	if (EOLCount<256 || EOLCount>5120 || EOLCountBad/EOLCount>3)
		if (!forceEOLcoding)
			TURN_OFF(OPTION_EOL_CODING);


	if (EOLCount>256 && EOLCount<5120 && EOLCountBad/EOLCount<6 && spaceAfterLF>1000)
	{
		TURN_ON(OPTION_EOL_CODING);
		TURN_ON(OPTION_SPACE_AFTER_EOL);
	}

	if (!IF_OPTION(OPTION_USE_DICTIONARY))
	{
		if (!forceWordSurroroundModeling)
			TURN_OFF(OPTION_WORD_SURROROUNDING_MODELING);
	}

/*	if (IF_OPTION(OPTION_USE_DICTIONARY))
		printf("ENG");
	if (IF_OPTION(OPTION_NORMAL_TEXT_FILTER))
		printf("text");
	if (IF_OPTION(OPTION_EOL_CODING))
		printf("+EOL");
	printf(" %s EOLCount=%d+%d(%d) binCount=%d/%d(%d) punctCount=%d/%d(%d) langSum=%d/%d(%d) SP_EOL=%d\n",filename,EOLCount,EOLCountBad,IF_OPTION(OPTION_EOL_CODING),binCount,part_length*parts/150,!IF_OPTION(OPTION_NORMAL_TEXT_FILTER),punctCount,punctCountBad,IF_OPTION(OPTION_WORD_SURROROUNDING_MODELING),langSum,part_length*parts/150,IF_OPTION(OPTION_USE_DICTIONARY),IF_OPTION(OPTION_SPACE_AFTER_EOL));
*/

//	PRINT_DICT(("%s EOLCount=%d+%d(%d) binCount=%d/%d(%d) punctCount=%d/%d(%d) langSum=%d/%d(%d) SP_EOL=%d\n",filename,EOLCount,EOLCountBad,IF_OPTION(OPTION_EOL_CODING),binCount,part_length*parts/150,!IF_OPTION(OPTION_NORMAL_TEXT_FILTER),punctCount,punctCountBad,IF_OPTION(OPTION_WORD_SURROROUNDING_MODELING),langSum,part_length*parts/150,IF_OPTION(OPTION_USE_DICTIONARY),IF_OPTION(OPTION_SPACE_AFTER_EOL)));

	return preprocFlag;
}



#ifdef WIN32
int getSourcePath(char* buf, int buf_size)
{
	int pos;

	pos=GetModuleFileName(NULL,buf,buf_size);

	for (int i=pos-1; i>=0; i--)
		if (buf[i]=='\\')
		{
			buf[i+1]=0;
			pos=i+1;
			break;
		}

	return pos;
}
#endif
